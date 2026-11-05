#include "VulkanDiagnostics.h"
#include "xrEngine/stdafx.h"
#include "VulkanGameDevice.h"
#include "ShadowStability.h"
#include "ParticleVisual.h"
#include "VulkanRainRender.h"
#include "VulkanThunderboltRender.h"
#include "SceneShaders.h"
#include "ShaderModule.h"
#include "PostProcessShaders.h"
#include "TransparentDrawOrder.h"
#include "VulkanGameLightingMath.h"
#include "VulkanSettings.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/RenderVisual.h"
#include "xrCDB/Frustum.h"
#include "xrEngine/x_ray.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstring>
#if defined(XR_PLATFORM_ANDROID)
#include <SDL_syswm.h>
#include <android/native_window.h>
#endif

namespace xray::render::vulkan
{
namespace
{
// Attribute CPU recording time to actual render passes in the normal device
// log. Aggregate CPU p95 also includes acquisition/fence waits.
struct PassCpuSample
{
    const char *name;
    const DeferredPass &pass;
    uint32_t draws;
    std::chrono::steady_clock::time_point begin{std::chrono::steady_clock::now()};
    ~PassCpuSample()
    {
        if (Device.dwFrame % (diagnostics().requested ? 30 : 300) == 0)
            Msg("[renderer-vulkan] pass.cpu frame=%u pass=%s ms=%.3f draws=%u", Device.dwFrame, name,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count(),
                pass.draw_calls() - draws);
    }
};

} // namespace

void VulkanGameDevice::record_distortion(const FrameRecordingContext &frame, void *user)
{
    if (diagnostics().mode == 9 || diagnostics().mode == 12) return;
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    auto command_batch = owner.deferred_.geometry_batch();
    for (auto &draw : owner.particle_draws_)
    {
        if (draw.hud)
            continue;
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        const bool ok = draw.visual->getType() == 8
            ? static_cast<VulkanParticleEffect *>(draw.visual)->record(
                  frame, owner.deferred_, mvp, draw.right, draw.up, false, owner.model_error_, GeometryPhase::Distortion)
            : static_cast<VulkanParticleGroup *>(draw.visual)->record(
                  frame, owner.deferred_, mvp, draw.right, draw.up, false, owner.model_error_, GeometryPhase::Distortion);
        owner.models_recorded_ &= ok;
    }
}

void VulkanGameDevice::capture_ui(VkCommandBuffer command, VkImage image, uint32_t index, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    owner.ui_recorded_ &= owner.water_targets_.capture(command, image, index);
}

void VulkanGameDevice::record_ui_distortion(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    owner.ui_recorded_ &= owner.ui_.record(frame, owner.ui_error_, true, owner.ui_scene_sets_.at(frame.image_index));
}

void VulkanGameDevice::record_ui(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    owner.ui_recorded_ &= owner.ui_.record(
        frame, owner.ui_error_, false, VK_NULL_HANDLE,
        frame.image_index < owner.ui_depth_sets_.size() ? owner.ui_depth_sets_[frame.image_index] : VK_NULL_HANDLE);
    if (!owner.ui_recorded_)
        Msg("! [renderer-vulkan] ui.record failed image=%u frame=%u overlay=%d draws=%u reason=%s", frame.image_index,
            frame.frame_index, frame.render_pass == owner.window_.frame().overlay_render_pass() ? 1 : 0,
            owner.last_ui_draw_calls_, owner.ui_error_.c_str());
}

void VulkanGameDevice::record_postprocess(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    owner.postprocess_passes_[frame.image_index]->record(frame);
}

void VulkanGameDevice::record_level_visuals(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    PassCpuSample timing{"opaque-level", owner.deferred_, owner.deferred_.draw_calls()};
    auto command_batch = owner.deferred_.geometry_batch();
    for (const auto &draw : owner.level_draws_)
    {
        if (draw.hud)
            continue;
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (!owner.current_level_->record_visual(draw.index, frame, owner.deferred_, mvp,
                                                 GeometryPhase::OpaqueAndAlphaTest, draw.lod, draw.instance == nullptr))
        {
            owner.level_recorded_ = false;
            owner.level_error_ = "Vulkan opaque level visual " + std::to_string(draw.index) +
                                 " failed (lod=" + std::to_string(draw.lod) + ")";
            return;
        }
    }
    float mvp[16];
    std::copy(owner.scene_mvp_.begin(), owner.scene_mvp_.end(), mvp);
    if (!owner.current_level_->record_details(frame, owner.deferred_, mvp))
    {
        owner.level_recorded_ = false;
        owner.level_error_ = "Vulkan level detail recording failed";
    }
}

void VulkanGameDevice::record_transparent(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    PassCpuSample timing{"transparent", owner.deferred_, owner.deferred_.draw_calls()};
    auto command_batch = owner.deferred_.geometry_batch();
    if (frame.image_index >= owner.forward_buffers_.size() ||
        !owner.forward_buffers_[frame.image_index].write(0, &owner.forward_lighting_, sizeof(owner.forward_lighting_),
                                                         owner.model_error_))
    {
        owner.models_recorded_ = false;
        return;
    }
    // R2 draws secondary priority ordinary geometry after combine and before
    // the sorted forward list. This pass replaces color and writes depth.
    if (owner.current_level_)
        for (const auto &level : owner.level_draws_)
        {
            if (level.hud || !owner.current_level_->has_secondary_priority(level.index))
                continue;
            float mvp[16];
            std::copy(level.mvp.begin(), level.mvp.end(), mvp);
            auto command_batch = owner.deferred_.geometry_batch();
            if (!owner.current_level_->record_visual(level.index, frame, owner.deferred_, mvp,
                                                     GeometryPhase::Transparent, level.lod, level.instance == nullptr,
                                                     true))
            {
                owner.level_recorded_ = false;
                owner.level_error_ =
                    "Vulkan secondary-priority level visual " + std::to_string(level.index) + " failed";
                return;
            }
        }
    for (auto &model : owner.model_draws_)
    {
        if (model.hud || !model.model->has_secondary_priority())
            continue;
        float mvp[16];
        owner.deferred_.set_model_hemi_negative(model.hemi_negative.data());
        std::copy(model.mvp.begin(), model.mvp.end(), mvp);
        const bool ok =
            model.skeleton
                ? model.model->record_animated(frame, owner.deferred_, mvp, *model.skeleton, owner.model_error_,
                                               GeometryPhase::SecondaryOpaque, model.lod, model.normal_rows.data())
                : model.model->record(frame, owner.deferred_, mvp, nullptr, 0, owner.model_error_,
                                      GeometryPhase::SecondaryOpaque, model.lod, nullptr, model.normal_rows.data());
        if (!ok)
        {
            owner.models_recorded_ = false;
            return;
        }
    }
    std::vector<TransparentDrawRef> draws;
    for (size_t i = 0; i < owner.level_draws_.size(); ++i)
        if (!owner.level_draws_[i].hud && owner.current_level_)
            for (uint8_t priority = 1; priority <= 3; ++priority)
                if (owner.current_level_->has_transparent_priority(owner.level_draws_[i].index, priority))
                    draws.push_back({owner.level_draws_[i].sort_distance, false, i, false, false, priority});
    for (size_t i = 0; i < owner.model_draws_.size(); ++i)
        if (!owner.model_draws_[i].hud)
            for (uint8_t priority = 1; priority <= 3; ++priority)
                if (owner.model_draws_[i].model->has_transparent_priority(priority))
                    draws.push_back({owner.model_draws_[i].sort_distance, true, i, false, false, priority});
    for (size_t i = 0; i < owner.particle_draws_.size(); ++i)
    {
        const auto &particle = owner.particle_draws_[i];
        if (particle.hud)
            continue;
        if (particle.visual->getType() == 8)
            draws.push_back({particle.sort_distance, false, i, true, false,
                             static_cast<VulkanParticleEffect *>(particle.visual)->priority()});
        else
        {
            const auto *group = static_cast<VulkanParticleGroup *>(particle.visual);
            for (uint8_t priority = 1; priority <= 3; ++priority)
                if (group->has_priority(priority))
                    draws.push_back({particle.sort_distance, false, i, true, false, priority});
        }
    }
    if (owner.current_level_)
    {
        float mvp[16];
        std::copy(owner.scene_mvp_.begin(), owner.scene_mvp_.end(), mvp);
        std::vector<float> detail_distances;
        if (!owner.current_level_->prepare_transparent_details(frame, mvp, detail_distances))
        {
            owner.level_recorded_ = false;
            owner.level_error_ = "Vulkan transparent level detail preparation failed";
            return;
        }
        for (size_t i = 0; i < detail_distances.size(); ++i)
            draws.push_back({detail_distances[i], false, i, false, true});
    }
    sort_transparent_draws(draws);
#if defined(XR_PLATFORM_ANDROID)
    if (Device.dwFrame % (diagnostics().requested ? 30 : 300) == 0)
    {
        size_t counts[4][3]{}; // priority 1..3: level, model, particle
        bool ordered = true;
        for (size_t i = 0; i < draws.size(); ++i)
        {
            const auto &draw = draws[i];
            if (draw.priority >= 1 && draw.priority <= 3)
                ++counts[draw.priority][draw.particle ? 2 : draw.model ? 1 : 0];
            if (i && (draws[i - 1].priority > draw.priority ||
                      (draws[i - 1].priority == draw.priority && draws[i - 1].distance < draw.distance)))
                ordered = false;
        }
        Msg("[renderer-vulkan] N2.priority frame=%u sorted=%d refs=%zu p1=%zu/%zu/%zu p2=%zu/%zu/%zu p3=%zu/%zu/%zu "
            "(level/model/particle)",
            Device.dwFrame, ordered ? 1 : 0, draws.size(), counts[1][0], counts[1][1], counts[1][2], counts[2][0],
            counts[2][1], counts[2][2], counts[3][0], counts[3][1], counts[3][2]);
    }
#endif
    for (const TransparentDrawRef &draw : draws)
    {
        float mvp[16];
        if (draw.detail)
        {
            std::copy(owner.scene_mvp_.begin(), owner.scene_mvp_.end(), mvp);
            if (!owner.current_level_->record_transparent_detail(draw.index, frame, owner.deferred_, mvp))
            {
                owner.level_recorded_ = false;
                owner.level_error_ = "Vulkan transparent level detail recording failed";
                return;
            }
        }
        else if (draw.particle)
        {
            auto &particle = owner.particle_draws_[draw.index];
            std::copy(particle.mvp.begin(), particle.mvp.end(), mvp);
            bool ok =
                particle.visual->getType() == 8
                    ? static_cast<VulkanParticleEffect *>(particle.visual)
                          ->record(frame, owner.deferred_, mvp, particle.right, particle.up, false, owner.model_error_)
                    : static_cast<VulkanParticleGroup *>(particle.visual)
                          ->record(frame, owner.deferred_, mvp, particle.right, particle.up, false, owner.model_error_,
                                   GeometryPhase::Transparent, draw.priority);
            if (!ok)
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else if (draw.model)
        {
            ModelDraw &model = owner.model_draws_[draw.index];
            owner.deferred_.set_model_hemi_negative(model.hemi_negative.data());
            std::copy(model.mvp.begin(), model.mvp.end(), mvp);
            if (model.skeleton ? !model.model->record_animated(frame, owner.deferred_, mvp, *model.skeleton,
                                                               owner.model_error_, GeometryPhase::Transparent,
                                                               model.lod, model.normal_rows.data(), draw.priority)
                               : !model.model->record(frame, owner.deferred_, mvp, nullptr, 0, owner.model_error_,
                                                      GeometryPhase::Transparent, model.lod, nullptr,
                                                      model.normal_rows.data(), draw.priority))
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else
        {
            const LevelDraw &level = owner.level_draws_[draw.index];
            std::copy(level.mvp.begin(), level.mvp.end(), mvp);
            auto command_batch = owner.deferred_.geometry_batch();
            if (!owner.current_level_->record_visual(level.index, frame, owner.deferred_, mvp,
                                                     GeometryPhase::Transparent, level.lod, level.instance == nullptr,
                                                     false, draw.priority))
            {
                owner.level_recorded_ = false;
                owner.level_error_ = "Vulkan transparent level visual " + std::to_string(level.index) +
                                     " failed (lod=" + std::to_string(level.lod) + ")";
                return;
            }
        }
    }
    for (VulkanRainRender *rain : owner.rain_draws_)
        if (rain && !rain->record(frame, owner.model_error_))
        {
            owner.models_recorded_ = false;
            return;
        }
    for (VulkanThunderboltRender *bolt : owner.thunderbolt_draws_)
        if (bolt && !bolt->record(frame, owner.model_error_))
        {
            owner.models_recorded_ = false;
            return;
        }
    // World-space IUIRender effects belong before the HUD clears depth and
    // paints the weapon. Overlay UI is recorded separately after the HUD.
    owner.ui_recorded_ &= owner.ui_.record(
        frame, owner.ui_error_, false, VK_NULL_HANDLE,
        frame.image_index < owner.ui_depth_sets_.size() ? owner.ui_depth_sets_[frame.image_index] : VK_NULL_HANDLE,
        true);
}

void VulkanGameDevice::prepare_light_grid(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    if (!use_light_grid)
    {
        owner.light_grid_.discard_frame(frame.image_index);
        return;
    }
    if (!owner.light_grid_.prepare(frame, owner.local_uniforms_, owner.local_scissors_, owner.model_error_))
    {
        owner.light_grid_.discard_frame(frame.image_index);
        if (Device.dwFrame % (diagnostics().requested ? 30 : 300) == 0)
            Msg("! [renderer-vulkan] light grid frame fallback: %s", owner.model_error_.c_str());
    }
}

void VulkanGameDevice::record_local_lights(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    PassCpuSample timing{"local-lights", owner.deferred_, owner.deferred_.draw_calls()};
    if (frame.image_index >= owner.local_sets_.size())
    {
        owner.local_lights_recorded_ = false;
        return;
    }
    if (owner.local_bound_projections_.size() != owner.local_sets_.size())
        owner.local_bound_projections_.resize(owner.local_sets_.size());
    if (!owner.local_lights_recorded_ ||
        !owner.light_grid_.record(frame, owner.targets_.lighting_set(frame.image_index)))
    {
        owner.local_lights_recorded_ = false;
        return;
    }
    if (owner.light_grid_.count(frame.image_index))
        owner.deferred_.account_light_grid_draw();
    if (Device.dwFrame % (diagnostics().requested ? 30 : 300) == 0)
        Msg("[renderer-vulkan] light.grid frame=%u point-lights=%u total=%zu", Device.dwFrame,
            owner.light_grid_.count(frame.image_index), owner.local_uniforms_.size());
    for (uint32_t i = 0; i < owner.local_uniforms_.size(); ++i)
    {
        if (owner.light_grid_.contains(frame.image_index, i))
            continue;
        VkDescriptorSet &set = owner.local_sets_[frame.image_index][i];
        auto &bound = owner.local_bound_projections_[frame.image_index][i];
        if (!set)
            bound = VK_NULL_HANDLE;
        if (!set && !owner.deferred_.local_light_set(
                        owner.local_shadows_.view(frame.image_index), owner.local_shadows_.sampler(),
                        owner.local_shadows_.uniform(frame.image_index),
                        VkDeviceSize(i) * owner.local_shadows_.uniform_stride(), sizeof(LocalLightUniform), set,
                        owner.model_error_, owner.local_projection_views_[i]))
        {
            owner.local_lights_recorded_ = false;
            return;
        }
        if (bound != owner.local_projection_views_[i])
        {
            owner.deferred_.update_light_projection(set, owner.local_projection_views_[i], owner.textures_.sampler());
            bound = owner.local_projection_views_[i];
        }
        VkRect2D scissor = owner.local_scissors_[i];
        if (owner.local_uniforms_[i].debug_mode[0] == 1.f)
        {
            // Project each light's scissor into the upper-left base panel.
            // Round the far edge up so edge pixels cannot lose their light.
            const uint32_t right = uint32_t(scissor.offset.x) + scissor.extent.width;
            const uint32_t bottom = uint32_t(scissor.offset.y) + scissor.extent.height;
            scissor.offset.x /= 2;
            scissor.offset.y /= 2;
            scissor.extent.width = (right + 1) / 2 - uint32_t(scissor.offset.x);
            scissor.extent.height = (bottom + 1) / 2 - uint32_t(scissor.offset.y);
        }
        if (!owner.deferred_.record_local_light(frame, owner.targets_.lighting_set(frame.image_index), set, scissor))
        {
            owner.local_lights_recorded_ = false;
            return;
        }
    }
}

void VulkanGameDevice::record_hud(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    struct DrawRef
    {
        float distance{};
        bool model{};
        size_t index{};
        bool particle{};
    };
    std::vector<DrawRef> hud;
    for (size_t i = 0; i < owner.level_draws_.size(); ++i)
        if (owner.level_draws_[i].hud)
            hud.push_back({owner.level_draws_[i].sort_distance, false, i});
    for (size_t i = 0; i < owner.model_draws_.size(); ++i)
        if (owner.model_draws_[i].hud)
            hud.push_back({owner.model_draws_[i].sort_distance, true, i});
    for (size_t i = 0; i < owner.particle_draws_.size(); ++i)
        if (owner.particle_draws_[i].hud)
            hud.push_back({owner.particle_draws_[i].sort_distance, false, i, true});
    std::stable_sort(hud.begin(), hud.end(),
                     [](const DrawRef &left, const DrawRef &right) { return left.distance > right.distance; });
    for (const DrawRef &draw : hud)
    {
        float mvp[16];
        if (draw.particle)
        {
            auto &particle = owner.particle_draws_[draw.index];
            std::copy(particle.mvp.begin(), particle.mvp.end(), mvp);
            bool ok =
                particle.visual->getType() == 8
                    ? static_cast<VulkanParticleEffect *>(particle.visual)
                          ->record(frame, owner.deferred_, mvp, particle.right, particle.up, true, owner.model_error_)
                    : static_cast<VulkanParticleGroup *>(particle.visual)
                          ->record(frame, owner.deferred_, mvp, particle.right, particle.up, true, owner.model_error_);
            if (!ok)
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else if (draw.model)
        {
            ModelDraw &model = owner.model_draws_[draw.index];
            owner.deferred_.set_model_hemi_negative(model.hemi_negative.data());
            std::copy(model.mvp.begin(), model.mvp.end(), mvp);
            const bool recorded =
                model.skeleton
                    ? model.model->record_animated(frame, owner.deferred_, mvp, *model.skeleton, owner.model_error_,
                                                   GeometryPhase::Hud, model.lod, model.normal_rows.data())
                    : model.model->record(frame, owner.deferred_, mvp, nullptr, 0, owner.model_error_,
                                          GeometryPhase::Hud, model.lod, nullptr, model.normal_rows.data());
            if (!recorded)
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else
        {
            const LevelDraw &level = owner.level_draws_[draw.index];
            std::copy(level.mvp.begin(), level.mvp.end(), mvp);
            if (!owner.current_level_->record_hud_visual(level.index, frame, owner.deferred_, mvp, level.lod))
            {
                owner.level_recorded_ = false;
                owner.level_error_ = "Vulkan HUD level visual " + std::to_string(level.index) +
                                     " failed (lod=" + std::to_string(level.lod) + ")";
                return;
            }
        }
    }
}

void VulkanGameDevice::record_shadow_models(const FrameRecordingContext &frame, const float (&vp)[16], void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    PassCpuSample timing{"shadow-models", owner.deferred_, owner.deferred_.draw_calls()};
    Fmatrix light;
    std::memcpy(&light, vp, sizeof(light));
    for (auto &draw : owner.shadow_model_draws_)
    {
        if (draw.hud)
            continue;
        Fmatrix world, mvp;
        std::memcpy(&world, draw.world.data(), sizeof(world));
        mvp.mul(light, world);
        if (draw.skeleton)
        {
            draw.skeleton->CalculateBones();
            const auto &sphere = draw.skeleton->dcast_RenderVisual()->getVisData().sphere;
            if (std::isfinite(sphere.R) && sphere.R > 0)
            {
                CFrustum frustum;
                frustum.CreateFromMatrix(mvp, FRUSTUM_P_LRTB | FRUSTUM_P_NEAR | FRUSTUM_P_FAR);
                if (!frustum.testSphere_dirty(sphere.P, sphere.R))
                    continue;
            }
        }
        float transform[16];
        std::memcpy(transform, &mvp, sizeof(mvp));
        if (!draw.model->record_shadow(frame, owner.deferred_, transform, draw.skeleton, owner.model_error_, draw.lod))
        {
            owner.models_recorded_ = false;
            return;
        }
    }
}

void VulkanGameDevice::record_models(const FrameRecordingContext &frame, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    PassCpuSample timing{"opaque-models", owner.deferred_, owner.deferred_.draw_calls()};
    auto command_batch = owner.deferred_.geometry_batch();
    for (auto &draw : owner.model_draws_)
    {
        if (draw.hud)
            continue;
        owner.deferred_.set_model_hemi_negative(draw.hemi_negative.data());
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (draw.skeleton
                ? !draw.model->record_animated(frame, owner.deferred_, mvp, *draw.skeleton, owner.model_error_,
                                               GeometryPhase::OpaqueAndAlphaTest, draw.lod, draw.normal_rows.data())
                : !draw.model->record(frame, owner.deferred_, mvp, nullptr, 0, owner.model_error_,
                                      GeometryPhase::OpaqueAndAlphaTest, draw.lod, nullptr, draw.normal_rows.data()))
        {
            owner.models_recorded_ = false;
            return;
        }
    }
    // R2's PARTICLE SET is a cutout G-buffer draw. Particle groups can mix
    // this mode with forward effects; each child filters itself by phase.
    for (auto &particle : owner.particle_draws_)
    {
        if (particle.hud)
            continue;
        float mvp[16];
        std::copy(particle.mvp.begin(), particle.mvp.end(), mvp);
        const bool ok = particle.visual->getType() == 8
                            ? static_cast<VulkanParticleEffect *>(particle.visual)
                                  ->record(frame, owner.deferred_, mvp, particle.right, particle.up, false,
                                           owner.model_error_, GeometryPhase::OpaqueAndAlphaTest)
                            : static_cast<VulkanParticleGroup *>(particle.visual)
                                  ->record(frame, owner.deferred_, mvp, particle.right, particle.up, false,
                                           owner.model_error_, GeometryPhase::OpaqueAndAlphaTest);
        if (!ok)
        {
            owner.models_recorded_ = false;
            return;
        }
    }
}

void VulkanGameDevice::record_readback(VkCommandBuffer command, VkImage image, VkExtent2D extent, void *user)
{
    auto &owner = *static_cast<VulkanGameDevice *>(user);
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {extent.width, extent.height, 1};
    owner.frame_dispatch_.cmd_copy_image_to_buffer(command, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                                   owner.screenshot_buffer_.handle(), 1, &region);
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = owner.screenshot_buffer_.handle();
    barrier.size = VK_WHOLE_SIZE;
    owner.frame_dispatch_.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0,
                                               0, nullptr, 1, &barrier, 0, nullptr);
}
} // namespace xray::render::vulkan
