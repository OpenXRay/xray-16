#include "xrEngine/stdafx.h"
#include "VulkanGameDevice.h"
#include "ParticleVisual.h"
#include "VulkanRainRender.h"
#include "VulkanThunderboltRender.h"
#include "SceneShaders.h"
#include "ShaderModule.h"
#include "PostProcessShaders.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
namespace
{
template <typename T> T proc(VkDevice device, PFN_vkGetDeviceProcAddr get, const char* name)
{
    return reinterpret_cast<T>(get(device, name));
}
}

bool VulkanGameDevice::initialize(SDL_Window* window, VkExtent2D extent, std::string& error)
{
    destroy();
    readback_enabled_ = window_.initialize(window, extent, true, error, true, true, true);
    if (!readback_enabled_) return false;
    const VkDevice device = window_.device();
    const auto get = window_.device_proc();
    const auto& physical = window_.physical();
    const auto& frame = window_.frame();
    if (!load_frame_dispatch(window_.instance(), window_.instance_proc(), device, get,
            frame_dispatch_, error)) goto failed;

#define LOAD_TEXTURE(field, name) texture_dispatch_.field = proc<decltype(texture_dispatch_.field)>(device, get, name)
    LOAD_TEXTURE(create_buffer, "vkCreateBuffer");
    LOAD_TEXTURE(destroy_buffer, "vkDestroyBuffer");
    LOAD_TEXTURE(get_buffer_memory_requirements, "vkGetBufferMemoryRequirements");
    LOAD_TEXTURE(create_image, "vkCreateImage");
    LOAD_TEXTURE(destroy_image, "vkDestroyImage");
    LOAD_TEXTURE(get_image_memory_requirements, "vkGetImageMemoryRequirements");
    LOAD_TEXTURE(allocate_memory, "vkAllocateMemory");
    LOAD_TEXTURE(free_memory, "vkFreeMemory");
    LOAD_TEXTURE(bind_buffer_memory, "vkBindBufferMemory");
    LOAD_TEXTURE(bind_image_memory, "vkBindImageMemory");
    LOAD_TEXTURE(map_memory, "vkMapMemory");
    LOAD_TEXTURE(unmap_memory, "vkUnmapMemory");
    LOAD_TEXTURE(create_image_view, "vkCreateImageView");
    LOAD_TEXTURE(destroy_image_view, "vkDestroyImageView");
    LOAD_TEXTURE(allocate_command_buffers, "vkAllocateCommandBuffers");
    LOAD_TEXTURE(free_command_buffers, "vkFreeCommandBuffers");
    LOAD_TEXTURE(begin_command_buffer, "vkBeginCommandBuffer");
    LOAD_TEXTURE(end_command_buffer, "vkEndCommandBuffer");
    LOAD_TEXTURE(cmd_pipeline_barrier, "vkCmdPipelineBarrier");
    LOAD_TEXTURE(cmd_copy_buffer_to_image, "vkCmdCopyBufferToImage");
    LOAD_TEXTURE(queue_submit, "vkQueueSubmit");
    LOAD_TEXTURE(create_fence, "vkCreateFence");
    LOAD_TEXTURE(destroy_fence, "vkDestroyFence");
    LOAD_TEXTURE(get_fence_status, "vkGetFenceStatus");
    LOAD_TEXTURE(wait_for_fences, "vkWaitForFences");
#undef LOAD_TEXTURE

    buffer_upload_.buffer = {
        texture_dispatch_.create_buffer, texture_dispatch_.destroy_buffer,
        texture_dispatch_.get_buffer_memory_requirements, texture_dispatch_.allocate_memory,
        texture_dispatch_.free_memory, texture_dispatch_.bind_buffer_memory,
        texture_dispatch_.map_memory, texture_dispatch_.unmap_memory};
    buffer_upload_.allocate_command_buffers = texture_dispatch_.allocate_command_buffers;
    buffer_upload_.free_command_buffers = texture_dispatch_.free_command_buffers;
    buffer_upload_.begin_command_buffer = texture_dispatch_.begin_command_buffer;
    buffer_upload_.end_command_buffer = texture_dispatch_.end_command_buffer;
    buffer_upload_.cmd_copy_buffer = proc<PFN_vkCmdCopyBuffer>(device, get, "vkCmdCopyBuffer");
    buffer_upload_.cmd_pipeline_barrier = texture_dispatch_.cmd_pipeline_barrier;
    buffer_upload_.queue_submit = texture_dispatch_.queue_submit;
    buffer_upload_.create_fence = texture_dispatch_.create_fence;
    buffer_upload_.destroy_fence = texture_dispatch_.destroy_fence;
    buffer_upload_.get_fence_status = texture_dispatch_.get_fence_status;
    buffer_upload_.wait_for_fences = texture_dispatch_.wait_for_fences;

    {
        const auto create_sampler = proc<PFN_vkCreateSampler>(device, get, "vkCreateSampler");
        const auto destroy_sampler = proc<PFN_vkDestroySampler>(device, get, "vkDestroySampler");
        create_sampler_ = create_sampler;
        destroy_sampler_ = destroy_sampler;
        ScenePassDispatch scene_dispatch;
        if (!load_scene_pass_dispatch(device, get, scene_dispatch, error) ||
            !create_sampler || !destroy_sampler || !buffer_upload_.cmd_copy_buffer)
        {
            if (error.empty()) error = "Vulkan game procedures are incomplete";
            goto failed;
        }
        if (!targets_.initialize(physical.handle, device, frame.extent(),
                static_cast<uint32_t>(frame.image_count()), frame.depth_format(),
                physical.memory, frame_dispatch_, create_sampler, destroy_sampler, error)) goto failed;
        if (!sun_shadows_.initialize(device, frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                physical.memory, frame_dispatch_, buffer_upload_.buffer,
                create_sampler, destroy_sampler, error)) goto failed;
        if (!local_shadows_.initialize(device, frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                physical.memory, frame_dispatch_, buffer_upload_.buffer,
                create_sampler, destroy_sampler, error,
                physical.properties.limits.minUniformBufferOffsetAlignment)) goto failed;
        local_sets_.resize(frame.image_count());
        {
            std::vector<VkImageView> depth_views;
            for (uint32_t i = 0; i < frame.image_count(); ++i)
                depth_views.push_back(targets_.depth_view(i));
            if (!window_.frame().attach_scene_depth(depth_views, error)) goto failed;
        }
        if (!window_.frame().enable_interpass(error) ||
            !water_targets_.initialize(physical.handle, device, frame.format(), frame.extent(),
                static_cast<uint32_t>(frame.image_count()), physical.memory,
                frame_dispatch_, buffer_upload_.buffer, error)) goto failed;
        const ShaderModuleDispatch shaders{proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
                                           proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
        configure_engine_shader_resources(device, shaders, shader_resources_);
        DeferredShaderFactory deferred_factory;
        if (!deferred_factory.create(device, shaders, scene_dispatch, targets_.render_pass(), frame.render_pass(), deferred_, shader_resources_, error,
                sun_shadows_.render_pass(), local_shadows_.render_pass()) ||
            !targets_.bind_lighting(deferred_, error))
            goto failed;
        targets_.bind_sun_shadow(deferred_, sun_shadows_);
        if (!create_forward_targets(error)) goto failed;
        ShaderModule scene_vertex, scene_fragment, ui_vertex, ui_fragment;
        if (!scene_vertex.initialize(device, shaders, scene_shaders::SceneVertex, sizeof(scene_shaders::SceneVertex), error) ||
            !scene_fragment.initialize(device, shaders, scene_shaders::SceneFragment, sizeof(scene_shaders::SceneFragment), error) ||
            !ui_vertex.initialize(device, shaders, scene_shaders::UiVertex, sizeof(scene_shaders::UiVertex), error) ||
            !ui_fragment.initialize(device, shaders, scene_shaders::UiFragment, sizeof(scene_shaders::UiFragment), error) ||
            !ui_pass_.initialize(device, frame.render_pass(), scene_vertex.handle(), scene_fragment.handle(), ui_vertex.handle(), ui_fragment.handle(),
                                 scene_dispatch, error, true) ||
            !textures_.initialize(device, window_.queue(), frame.command_pool(), physical.memory, physical.features.textureCompressionBC, texture_dispatch_,
                                  create_sampler, destroy_sampler, frame_dispatch_.device_wait_idle, error) ||
            !create_postprocess(error))
            goto failed;
        for (uint32_t i = 0; i < frame.image_count(); ++i)
            if (!deferred_.water_set(i, water_targets_.refraction(i), water_targets_.reflection(i),
                    targets_.sampled_depth_view(i), textures_.sampler(), water_targets_.uniform(i), error)) goto failed;
    }
    deferred_.set_game_pipeline_request([this](const std::string &vs, const std::string &ps, SurfaceMode mode, bool hud, bool skinned, std::string &reason) {
        return request_shader_pair(vs, ps, mode, hud, skinned, reason);
    });
    ui_.configure(
        device, physical.memory, buffer_upload_.buffer, ui_pass_,
        {[this](VkDescriptorSet set) { return textures_.retain_ui(set, ui_pass_); }, [this](VkDescriptorSet set) { textures_.release_ui(set, ui_pass_); }});
    error.clear();
    return true;
failed:
    destroy();
    return false;
}

bool VulkanGameDevice::reload_game_shaders(std::string& error)
{
    const VkDevice device = window_.device();
    if (!device || !window_.frame().wait_idle())
    {
        error = "Vulkan shader reload needs an idle gameplay device";
        return false;
    }
    const auto get = window_.device_proc();
    const ShaderModuleDispatch shaders{proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
                                       proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
    DeferredShaderFactory factory;
    shader_resources_.clear();
    return factory.reload_game_pipelines(device, shaders, deferred_, shader_resources_, error);
}

void VulkanGameDevice::record_ui(const FrameRecordingContext &frame, void *user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    owner.ui_recorded_ = owner.ui_.record(frame, owner.ui_error_);
}

bool VulkanGameDevice::create_forward_targets(std::string& error)
{
    release_forward_targets();
    const auto& frame = window_.frame();
    forward_buffers_.resize(frame.image_count());
    for (uint32_t i = 0; i < frame.image_count(); ++i)
        if (!forward_buffers_[i].initialize(window_.device(), sizeof(ForwardLightUniform),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                window_.physical().memory, buffer_upload_.buffer, error) ||
            !deferred_.forward_set(i, forward_buffers_[i].handle(), error))
        { release_forward_targets(); return false; }
    return true;
}

void VulkanGameDevice::release_forward_targets()
{
    deferred_.release_forward_sets();
    forward_buffers_.clear();
}

bool VulkanGameDevice::create_postprocess(std::string& error)
{
    postprocess_passes_.clear();
    auto& frame = window_.frame();
    const auto device = window_.device();
    const auto get = window_.device_proc();
    ScreenCopyDispatch dispatch;
    ShaderModuleDispatch modules{proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
        proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
    ShaderModule vertex, fragment;
    if (!load_screen_copy_dispatch(device, get, dispatch, error) || !dispatch.cmd_push_constants ||
        !vertex.initialize(device, modules, postprocess_shaders::Vertex,
            sizeof(postprocess_shaders::Vertex), error) ||
        !fragment.initialize(device, modules, postprocess_shaders::Fragment,
            sizeof(postprocess_shaders::Fragment), error))
    {
        if (error.empty()) error = "Vulkan postprocess requires push constants";
        return false;
    }
    for (size_t i = 0; i < frame.image_count(); ++i)
    {
        auto pass = std::make_unique<ScreenCopyPass>();
        if (!pass->initialize(device, frame.composite_render_pass(), frame.postprocess_view(i),
                textures_.sampler(), vertex.handle(), fragment.handle(), dispatch, error))
        {
            postprocess_passes_.clear();
            return false;
        }
        pass->set_color_maps(color_map_views_[0], color_map_views_[1]);
        postprocess_passes_.push_back(std::move(pass));
    }
    return true;
}

void VulkanGameDevice::set_postprocess(const PostProcessConstants& params,
    std::string first, std::string second)
{
    postprocess_params_ = params;
    color_map_names_[0] = std::move(first);
    color_map_names_[1] = std::move(second);
}

bool VulkanGameDevice::refresh_color_maps(std::string& error)
{
    if (color_map_names_[0] == active_map_names_[0] &&
        color_map_names_[1] == active_map_names_[1]) return true;
    // Descriptor writes and texture eviction must wait for previous submissions.
    if (!window_.frame().wait_idle())
    {
        error = "could not wait for previous frames before changing postprocess color maps";
        return false;
    }
    VkImageView replacements[2]{};
    for (size_t i = 0; i < 2; ++i)
        if (!color_map_names_[i].empty() &&
            !textures_.environment(color_map_names_[i], replacements[i], error))
        {
            for (auto view : replacements) if (view) textures_.release_environment(view);
            return false;
        }
    if (!textures_.finish_uploads())
    {
        for (auto view : replacements) if (view) textures_.release_environment(view);
        error = "could not upload Vulkan postprocess color maps";
        return false;
    }
    for (auto& pass : postprocess_passes_)
        pass->set_color_maps(replacements[0], replacements[1]);
    for (auto view : color_map_views_) if (view) textures_.release_environment(view);
    std::copy_n(replacements, 2, color_map_views_);
    std::copy_n(color_map_names_, 2, active_map_names_);
    return true;
}

void VulkanGameDevice::record_postprocess(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    owner.postprocess_passes_[frame.image_index]->record(frame);
}

void VulkanGameDevice::queue_model(GpuModel& model, IKinematics* skeleton, const float (&mvp)[16],
    bool hud, float sort_distance, const void* instance, float lod)
{
    ModelDraw draw;
    draw.model = &model;
    draw.instance = instance;
    draw.skeleton = skeleton;
    std::copy_n(mvp, 16, draw.mvp.data());
    draw.sort_distance = std::isfinite(sort_distance) ? std::max(sort_distance, 0.f) : 0.f;
    draw.lod = lod;
    draw.hud = hud;
    model_draws_.push_back(draw);
}

void VulkanGameDevice::queue_particle(IRenderVisual* visual, const float (&mvp)[16],
    const Fvector& right, const Fvector& up, bool hud, float distance)
{
    ParticleDraw draw;
    draw.visual = visual;
    std::copy_n(mvp, 16, draw.mvp.data());
    draw.right = right;
    draw.up = up;
    draw.hud = hud;
    draw.sort_distance = std::isfinite(distance) ? std::max(distance, 0.f) : 0.f;
    particle_draws_.push_back(draw);
}

void VulkanGameDevice::discard_model_draws(const void* instance)
{
    particle_draws_.erase(std::remove_if(particle_draws_.begin(), particle_draws_.end(),
                              [instance](const ParticleDraw& draw) { return draw.visual == instance; }),
        particle_draws_.end());
    model_draws_.erase(std::remove_if(model_draws_.begin(), model_draws_.end(),
                           [instance](const ModelDraw& draw)
                           {
                               return draw.instance == instance;
                           }),
        model_draws_.end());
    level_draws_.erase(std::remove_if(level_draws_.begin(), level_draws_.end(),
                           [instance](const LevelDraw& draw) { return draw.instance == instance; }),
        level_draws_.end());
}

void VulkanGameDevice::queue_level_visual(uint32_t index, const float (&mvp)[16],
    bool hud, float sort_distance, const void* instance, float lod)
{
    const auto duplicate = std::find_if(level_draws_.begin(), level_draws_.end(),
        [index, hud, instance, &mvp](const LevelDraw& draw)
        {
            return draw.index == index && draw.hud == hud && draw.instance == instance &&
                std::equal(draw.mvp.begin(), draw.mvp.end(), mvp);
        });
    if (duplicate != level_draws_.end())
        return;

    LevelDraw draw;
    draw.index = index;
    draw.instance = instance;
    std::copy_n(mvp, 16, draw.mvp.data());
    draw.sort_distance = std::isfinite(sort_distance) ? std::max(sort_distance, 0.f) : 0.f;
    draw.lod = lod;
    draw.hud = hud;
    level_draws_.push_back(draw);
}

void VulkanGameDevice::record_level_visuals(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    for (const auto& draw : owner.level_draws_)
    {
        if (draw.hud) continue;
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (!owner.current_level_->record_visual(draw.index, frame, owner.deferred_, mvp,
                GeometryPhase::OpaqueAndAlphaTest, draw.lod))
        {
            owner.level_recorded_ = false;
            return;
        }
    }
    float mvp[16];
    std::copy(owner.scene_mvp_.begin(), owner.scene_mvp_.end(), mvp);
    if (!owner.current_level_->record_details(frame, owner.deferred_, mvp))
        owner.level_recorded_ = false;
}

void VulkanGameDevice::record_transparent(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    if (frame.image_index >= owner.forward_buffers_.size() ||
        !owner.forward_buffers_[frame.image_index].write(0, &owner.forward_lighting_,
            sizeof(owner.forward_lighting_), owner.model_error_))
    { owner.models_recorded_ = false; return; }
    struct DrawRef
    {
        float distance{};
        bool model{};
        size_t index{};
        bool particle{};
    };
    std::vector<DrawRef> draws;
    for (size_t i = 0; i < owner.level_draws_.size(); ++i)
        if (!owner.level_draws_[i].hud) draws.push_back({owner.level_draws_[i].sort_distance, false, i});
    for (size_t i = 0; i < owner.model_draws_.size(); ++i)
        if (!owner.model_draws_[i].hud) draws.push_back({owner.model_draws_[i].sort_distance, true, i});
    for (size_t i = 0; i < owner.particle_draws_.size(); ++i)
        if (!owner.particle_draws_[i].hud) draws.push_back({owner.particle_draws_[i].sort_distance, false, i, true});
    std::stable_sort(draws.begin(), draws.end(), [](const DrawRef& left, const DrawRef& right)
    {
        return left.distance > right.distance;
    });
    for (const DrawRef& draw : draws)
    {
        float mvp[16];
        if (draw.particle)
        {
            auto& particle = owner.particle_draws_[draw.index];
            std::copy(particle.mvp.begin(), particle.mvp.end(), mvp);
            bool ok = particle.visual->getType() == 8 ?
                static_cast<VulkanParticleEffect*>(particle.visual)->record(frame, owner.deferred_, mvp,
                    particle.right, particle.up, false, owner.model_error_) :
                static_cast<VulkanParticleGroup*>(particle.visual)->record(frame, owner.deferred_, mvp,
                    particle.right, particle.up, false, owner.model_error_);
            if (!ok) { owner.models_recorded_ = false; return; }
        }
        else if (draw.model)
        {
            ModelDraw& model = owner.model_draws_[draw.index];
            std::copy(model.mvp.begin(), model.mvp.end(), mvp);
            if (model.skeleton ?
                    !model.model->record_animated(frame, owner.deferred_, mvp,
                        *model.skeleton, owner.model_error_, GeometryPhase::Transparent, model.lod) :
                    !model.model->record(frame, owner.deferred_, mvp,
                        nullptr, 0, owner.model_error_, GeometryPhase::Transparent, model.lod))
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else
        {
            const LevelDraw& level = owner.level_draws_[draw.index];
            std::copy(level.mvp.begin(), level.mvp.end(), mvp);
            if (!owner.current_level_->record_visual(level.index, frame,
                    owner.deferred_, mvp, GeometryPhase::Transparent, level.lod))
            {
                owner.level_recorded_ = false;
                return;
            }
        }
    }
    for (VulkanRainRender* rain : owner.rain_draws_)
        if (rain && !rain->record(frame, owner.model_error_))
        {
            owner.models_recorded_ = false;
            return;
        }
    for (VulkanThunderboltRender* bolt : owner.thunderbolt_draws_)
        if (bolt && !bolt->record(frame, owner.model_error_))
        {
            owner.models_recorded_ = false;
            return;
        }
}

void VulkanGameDevice::record_local_lights(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    if (frame.image_index >= owner.local_sets_.size())
    { owner.local_lights_recorded_ = false; return; }
    for (uint32_t i = 0; i < owner.local_uniforms_.size(); ++i)
    {
        VkDescriptorSet& set = owner.local_sets_[frame.image_index][i];
        if (!set && !owner.deferred_.local_light_set(owner.local_shadows_.view(frame.image_index),
                owner.local_shadows_.sampler(), owner.local_shadows_.uniform(frame.image_index),
                VkDeviceSize(i) * owner.local_shadows_.uniform_stride(), sizeof(LocalLightUniform),
                set, owner.model_error_))
        { owner.local_lights_recorded_ = false; return; }
        if (!owner.deferred_.record_local_light(frame, owner.targets_.lighting_set(frame.image_index), set))
        { owner.local_lights_recorded_ = false; return; }
    }
}

void VulkanGameDevice::record_hud(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    struct DrawRef
    {
        float distance{};
        bool model{};
        size_t index{};
        bool particle{};
    };
    std::vector<DrawRef> hud;
    for (size_t i = 0; i < owner.level_draws_.size(); ++i)
        if (owner.level_draws_[i].hud) hud.push_back({owner.level_draws_[i].sort_distance, false, i});
    for (size_t i = 0; i < owner.model_draws_.size(); ++i)
        if (owner.model_draws_[i].hud) hud.push_back({owner.model_draws_[i].sort_distance, true, i});
    for (size_t i = 0; i < owner.particle_draws_.size(); ++i)
        if (owner.particle_draws_[i].hud) hud.push_back({owner.particle_draws_[i].sort_distance, false, i, true});
    std::stable_sort(hud.begin(), hud.end(), [](const DrawRef& left, const DrawRef& right)
    {
        return left.distance > right.distance;
    });
    for (const DrawRef& draw : hud)
    {
        float mvp[16];
        if (draw.particle)
        {
            auto& particle = owner.particle_draws_[draw.index];
            std::copy(particle.mvp.begin(), particle.mvp.end(), mvp);
            bool ok = particle.visual->getType() == 8 ?
                static_cast<VulkanParticleEffect*>(particle.visual)->record(frame, owner.deferred_, mvp,
                    particle.right, particle.up, true, owner.model_error_) :
                static_cast<VulkanParticleGroup*>(particle.visual)->record(frame, owner.deferred_, mvp,
                    particle.right, particle.up, true, owner.model_error_);
            if (!ok) { owner.models_recorded_ = false; return; }
        }
        else if (draw.model)
        {
            ModelDraw& model = owner.model_draws_[draw.index];
            std::copy(model.mvp.begin(), model.mvp.end(), mvp);
            const bool recorded = model.skeleton ?
                model.model->record_animated(frame, owner.deferred_, mvp,
                    *model.skeleton, owner.model_error_, GeometryPhase::Hud, model.lod) :
                model.model->record(frame, owner.deferred_, mvp,
                    nullptr, 0, owner.model_error_, GeometryPhase::Hud, model.lod);
            if (!recorded)
            {
                owner.models_recorded_ = false;
                return;
            }
        }
        else
        {
            const LevelDraw& level = owner.level_draws_[draw.index];
            std::copy(level.mvp.begin(), level.mvp.end(), mvp);
            if (!owner.current_level_->record_hud_visual(level.index, frame,
                    owner.deferred_, mvp, level.lod))
            {
                owner.level_recorded_ = false;
                return;
            }
        }
    }
}

void VulkanGameDevice::record_models(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    for (auto& draw : owner.model_draws_)
    {
        if (draw.hud) continue;
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (draw.skeleton ?
                !draw.model->record_animated(frame, owner.deferred_, mvp,
                    *draw.skeleton, owner.model_error_, GeometryPhase::OpaqueAndAlphaTest, draw.lod) :
                !draw.model->record(frame, owner.deferred_, mvp,
                    nullptr, 0, owner.model_error_, GeometryPhase::OpaqueAndAlphaTest, draw.lod))
        {
            owner.models_recorded_ = false;
            return;
        }
    }
}

void VulkanGameDevice::begin_frame()
{
    discard_scene_draws();
    light_snapshots_.clear();
    ui_.reset_frame();
    weather_set_ = VK_NULL_HANDLE;
}

VkDescriptorSet VulkanGameDevice::glow_texture(const std::string& name)
{
    if (name.empty()) return VK_NULL_HANDLE;
    const auto found = glow_textures_.find(name);
    if (found != glow_textures_.end()) return found->second;
    VkDescriptorSet set = VK_NULL_HANDLE;
    std::string error;
    if (!textures_.ui(name, ui_pass_, set, error))
    { Msg("! [renderer-vulkan] glow texture '%s': %s", name.c_str(), error.c_str()); return VK_NULL_HANDLE; }
    glow_textures_.emplace(name, set);
    return set;
}

void VulkanGameDevice::release_level_glows()
{
    if (glow_textures_.empty()) return;
    R_ASSERT2(wait_idle(), "Vulkan glow descriptors require idle GPU frames");
    for (const auto& [name, set] : glow_textures_)
        textures_.release_ui(set, ui_pass_);
    glow_textures_.clear();
}

void VulkanGameDevice::queue_weather(VkDescriptorSet set, const WeatherLighting& lighting)
{
    weather_set_ = set;
    weather_lighting_ = lighting;
}

void VulkanGameDevice::queue_rain(VulkanRainRender& rain)
{
    rain_draws_.push_back(&rain);
}

void VulkanGameDevice::queue_thunderbolt(VulkanThunderboltRender& bolt)
{
    thunderbolt_draws_.push_back(&bolt);
}

void VulkanGameDevice::discard_rain(const VulkanRainRender* rain)
{
    rain_draws_.erase(std::remove(rain_draws_.begin(), rain_draws_.end(), rain), rain_draws_.end());
}

void VulkanGameDevice::discard_thunderbolt(const VulkanThunderboltRender* bolt)
{
    thunderbolt_draws_.erase(std::remove(thunderbolt_draws_.begin(), thunderbolt_draws_.end(), bolt),
        thunderbolt_draws_.end());
}

void VulkanGameDevice::record_readback(VkCommandBuffer command, VkImage image,
    VkExtent2D extent, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {extent.width, extent.height, 1};
    owner.frame_dispatch_.cmd_copy_image_to_buffer(command, image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, owner.screenshot_buffer_.handle(), 1, &region);
    VkBufferMemoryBarrier barrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = owner.screenshot_buffer_.handle();
    barrier.size = VK_WHOLE_SIZE;
    owner.frame_dispatch_.cmd_pipeline_barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
}

bool VulkanGameDevice::take_screenshot(std::vector<uint8_t>& pixels,
    VkExtent2D& extent, VkFormat& format)
{
    if (!screenshot_ready_) return false;
    screenshot_ready_ = false;
    extent = screenshot_extent_;
    format = window_.frame().format();
    pixels.resize(size_t(extent.width) * extent.height * 4);
    std::string error;
    if (!screenshot_buffer_.read(0, pixels.data(), pixels.size(), error))
    { Msg("! [renderer-vulkan] screenshot: %s", error.c_str()); return false; }
    return true;
}

void VulkanGameDevice::discard_scene_draws()
{
    model_draws_.clear();
    particle_draws_.clear();
    rain_draws_.clear();
    thunderbolt_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
}

bool VulkanGameDevice::render(const GpuLevel& level, const float (&mvp)[16],
    const DeferredLight& light, FrameStatus& status, std::string& error,
    bool render_world, bool clear_target)
{
    ui_recorded_ = true;
    models_recorded_ = true;
    local_lights_recorded_ = true;
    level_recorded_ = true;
    current_level_ = &level;
    std::copy(std::begin(mvp), std::end(mvp), scene_mvp_.begin());
    ui_error_.clear();
    model_error_.clear();
    if (!refresh_color_maps(error)) return false;
    for (auto& pass : postprocess_passes_) pass->set_constants(postprocess_params_);
    const float weather_blend = weather_lighting_.light.color[3];
    weather_lighting_.light = light;
    weather_lighting_.light.color[3] = weather_blend;
    last_ui_draw_calls_ = ui_.draw_calls();
    last_ui_triangles_ = ui_.triangles();
    screenshot_ready_ = false;
    if (screenshot_requested_ && readback_enabled_)
    {
        screenshot_extent_ = window_.frame().extent();
        const VkDeviceSize bytes = VkDeviceSize(screenshot_extent_.width) * screenshot_extent_.height * 4;
        if (screenshot_buffer_.size() < bytes)
        {
            if (!window_.frame().wait_idle() || !screenshot_buffer_.initialize(window_.device(), bytes,
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, window_.physical().memory,
                    buffer_upload_.buffer, error)) return false;
        }
    }
    SunShadowUniform sun_uniform{};
    WaterSceneUniform water_uniform{};
    {
        Fmatrix camera, inverse, sun_view, sun_projection, sun_vp;
        std::memcpy(&camera, mvp, sizeof(camera));
        inverse.invert(camera);
        std::memcpy(sun_uniform.inverse_view_projection, &inverse, sizeof(inverse));
        std::memcpy(water_uniform.view_projection, &camera, sizeof(camera));
        std::memcpy(water_uniform.inverse_view_projection, &inverse, sizeof(inverse));
        std::memcpy(forward_lighting_.inverse_view_projection, &inverse, sizeof(inverse));
        water_uniform.camera_position[0] = Device.vCameraPosition.x;
        water_uniform.camera_position[1] = Device.vCameraPosition.y;
        water_uniform.camera_position[2] = Device.vCameraPosition.z;
        water_uniform.camera_position[3] = 1.f;
        Fvector direction;
        direction.set(light.direction_ambient[0], light.direction_ambient[1],
            light.direction_ambient[2]);
        if (direction.square_magnitude() < 0.0001f) direction.set(0.f, -1.f, 0.f);
        direction.normalize_safe();
        Fvector up;
        up.set(0.f, 1.f, 0.f);
        if (std::abs(direction.y) > .95f) up.set(0.f, 0.f, 1.f);
        Fvector origin;
        origin.mad(Device.vCameraPosition, direction, -140.f);
        sun_view.build_camera_dir(origin, direction, up);
        sun_projection.build_projection_ortho(180.f, 180.f, 1.f, 320.f);
        sun_vp.mul(sun_projection, sun_view);
        std::memcpy(sun_uniform.sun_view_projection, &sun_vp, sizeof(sun_vp));
    }
    local_uniforms_.clear();
    if (render_world)
    {
        Fmatrix camera, inverse;
        std::memcpy(&camera, mvp, sizeof(camera));
        inverse.invert(camera);
        uint32_t shadow_slot = 0;
        std::vector<const VulkanLightSnapshot*> nearby;
        nearby.reserve(light_snapshots_.size());
        for (const auto& snapshot : light_snapshots_)
            if (snapshot.active && std::isfinite(snapshot.range) && snapshot.range > .1f &&
                (snapshot.type == VulkanLightType::Point ||
                    snapshot.type == VulkanLightType::Spot ||
                    snapshot.type == VulkanLightType::OmniPart) &&
                std::all_of(snapshot.position.begin(), snapshot.position.end(),
                    [](float value) { return std::isfinite(value); }))
                nearby.push_back(&snapshot);
        const auto distance = [](const VulkanLightSnapshot* snapshot)
        {
            const float dx = snapshot->position[0] - Device.vCameraPosition.x;
            const float dy = snapshot->position[1] - Device.vCameraPosition.y;
            const float dz = snapshot->position[2] - Device.vCameraPosition.z;
            return dx * dx + dy * dy + dz * dz;
        };
        std::stable_sort(nearby.begin(), nearby.end(), [&](const auto* a, const auto* b)
        { return distance(a) < distance(b); });
        for (const VulkanLightSnapshot* snapshot : nearby)
        {
            if (local_uniforms_.size() >= LocalLightCapacity) break;
            const auto& light_snapshot = *snapshot;
            LocalLightUniform local{};
            std::memcpy(local.inverse_view_projection, &inverse, sizeof(inverse));
            const auto& position = light_snapshot.position;
            local.position_range[0] = position[0];
            local.position_range[1] = position[1];
            local.position_range[2] = position[2];
            local.position_range[3] = std::clamp(light_snapshot.range, .2f, 1000.f);
            for (uint32_t axis = 0; axis < 3; ++axis)
            {
                local.direction_cone[axis] = light_snapshot.direction[axis];
                local.color_type[axis] = std::isfinite(light_snapshot.color[axis]) ?
                    std::max(0.f, light_snapshot.color[axis]) : 0.f;
            }
            const bool spot = light_snapshot.type == VulkanLightType::Spot;
            local.color_type[3] = spot ? 1.f : 0.f;
            const float cone = std::clamp(light_snapshot.cone, .15f, 3.f);
            local.direction_cone[3] = std::cos(cone * .5f);
            const bool cast = light_snapshot.shadow && shadow_slot < LocalShadowSlots;
            local.shadow_params[0] = float(shadow_slot * LocalShadowFaces);
            local.shadow_params[1] = .002f;
            local.shadow_params[2] = cast ? 1.f : 0.f;
            if (cast)
            {
                const float directions[6][3]{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
                const float ups[6][3]{{0,-1,0},{0,-1,0},{0,0,1},{0,0,-1},{0,-1,0},{0,-1,0}};
                const uint32_t faces = spot ? 1 : LocalShadowFaces;
                for (uint32_t face = 0; face < faces; ++face)
                {
                    Fvector origin, direction, up;
                    origin.set(position[0], position[1], position[2]);
                    direction.set(directions[face][0], directions[face][1], directions[face][2]);
                    up.set(ups[face][0], ups[face][1], ups[face][2]);
                    if (spot)
                    {
                        direction.set(light_snapshot.direction[0], light_snapshot.direction[1],
                            light_snapshot.direction[2]).normalize_safe();
                        up.set(0.f, std::abs(direction.y) > .95f ? 0.f : 1.f,
                            std::abs(direction.y) > .95f ? 1.f : 0.f);
                    }
                    Fmatrix view, projection, vp;
                    view.build_camera_dir(origin, direction, up);
                    projection.build_projection(spot ? cone : 1.57079632679f,
                        1.f, .1f, local.position_range[3]);
                    vp.mul(projection, view);
                    std::memcpy(local.shadow_matrices[face], &vp, sizeof(vp));
                }
                ++shadow_slot;
            }
            local_uniforms_.push_back(local);
        }
    }
    std::copy_n(light.direction_ambient, 4, forward_lighting_.sun_direction_ambient);
    std::copy_n(light.color, 3, forward_lighting_.sun_color_count);
    const uint32_t forward_count = std::min<size_t>(local_uniforms_.size(), ForwardLightCapacity);
    forward_lighting_.sun_color_count[3] = static_cast<float>(forward_count);
    const VkExtent2D forward_extent = window_.frame().extent();
    forward_lighting_.viewport[0] = forward_extent.width ? 1.f / forward_extent.width : 0.f;
    forward_lighting_.viewport[1] = forward_extent.height ? 1.f / forward_extent.height : 0.f;
    for (uint32_t i = 0; i < forward_count; ++i)
    {
        std::copy_n(local_uniforms_[i].position_range, 4, forward_lighting_.local[i].position_range);
        std::copy_n(local_uniforms_[i].direction_cone, 4, forward_lighting_.local[i].direction_cone);
        std::copy_n(local_uniforms_[i].color_type, 4, forward_lighting_.local[i].color_type);
    }
    if (!frame_.render(window_.frame(), targets_, level, deferred_, mvp,
            light, status, error, record_ui, this, record_hud, this, record_models, this,
            record_transparent, this,
            scene_visibility_ ? record_level_visuals : nullptr, this, render_world, clear_target,
            render_world ? weather_set_ : VK_NULL_HANDLE, &weather_lighting_,
            screenshot_requested_ && readback_enabled_ ? record_readback : nullptr, this,
            record_postprocess, this, &sun_shadows_, &sun_uniform,
            &local_shadows_, &local_uniforms_, record_local_lights, this,
            render_world && level.has_water() ? &water_targets_ : nullptr, &water_uniform))
    {
        if (!window_.frame().device_lost())
            reset_required_ = true;
        model_draws_.clear();
        particle_draws_.clear();
        level_draws_.clear();
        current_level_ = nullptr;
        ui_.reset_frame();
        return false;
    }
    if (status == FrameStatus::RecreateRequired)
        reset_required_ = true;
    else if (screenshot_requested_ && readback_enabled_)
        screenshot_ready_ = window_.frame().wait_idle();
    screenshot_requested_ = false;
    model_draws_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
    // A successful submission consumes every CPU-side command, even when an
    // individual recorder reported malformed input after recording began.
    ui_.reset_frame();
    if (!level_recorded_)
    {
        error = "Vulkan level visual recording failed";
        return false;
    }
    if (!models_recorded_)
    {
        error = model_error_;
        return false;
    }
    if (!local_lights_recorded_)
    {
        error = model_error_.empty() ? "Vulkan local light recording failed" : model_error_;
        return false;
    }
    if (!ui_recorded_)
    {
        error = ui_error_;
        return false;
    }
    return true;
}

bool VulkanGameDevice::recreate_swapchain(VkExtent2D extent, std::string& error, bool recreate_surface)
{
    if (!window_.device() || !extent.width || !extent.height)
    {
        error = "Vulkan swapchain reset requires a live device and nonzero drawable extent";
        return false;
    }
    if (!window_.frame().wait_idle())
    {
        error = "could not wait for Vulkan device before renderer reset";
        return false;
    }

    const bool replace_surface = recreate_surface || window_.frame().surface_lost();
    targets_.release_lighting(deferred_);
    deferred_.release_water_sets();
    release_forward_targets();
    postprocess_passes_.clear();
    if (!window_.frame().release_swapchain())
    {
        error = "could not release Vulkan framebuffers before scene depth";
        return false;
    }
    targets_.destroy();
    water_targets_.destroy();
    if (!(replace_surface ? window_.recreate_surface(extent, error) :
              window_.recreate_frame(extent, error)))
        return false;

    auto& frame = window_.frame();
    const auto& physical = window_.physical();
    if (!targets_.initialize(physical.handle, window_.device(), frame.extent(),
            static_cast<uint32_t>(frame.image_count()), frame.depth_format(), physical.memory,
            frame_dispatch_, create_sampler_, destroy_sampler_, error))
        return false;
    sun_shadows_.destroy();
    if (!sun_shadows_.initialize(window_.device(), frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
            physical.memory, frame_dispatch_, buffer_upload_.buffer,
            create_sampler_, destroy_sampler_, error)) return false;
    for (auto& image : local_sets_)
        for (auto& set : image) deferred_.release_gbuffer(set);
    local_sets_.clear();
    local_shadows_.destroy();
    if (!local_shadows_.initialize(window_.device(), frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
            physical.memory, frame_dispatch_, buffer_upload_.buffer,
            create_sampler_, destroy_sampler_, error,
            physical.properties.limits.minUniformBufferOffsetAlignment)) return false;
    local_sets_.resize(frame.image_count());
    {
        std::vector<VkImageView> depth_views;
        for (uint32_t i = 0; i < frame.image_count(); ++i)
            depth_views.push_back(targets_.depth_view(i));
        if (!frame.attach_scene_depth(depth_views, error)) return false;
    }
    if (!frame.enable_interpass(error) ||
        !water_targets_.initialize(physical.handle, window_.device(), frame.format(), frame.extent(),
            static_cast<uint32_t>(frame.image_count()), physical.memory, frame_dispatch_,
            buffer_upload_.buffer, error))
        return false;
    deferred_.rebind_compatible_render_passes(targets_.render_pass(), frame.render_pass(),
        sun_shadows_.render_pass(), local_shadows_.render_pass());
    ui_pass_.rebind_render_pass(frame.render_pass());
    if (!targets_.bind_lighting(deferred_, error))
        return false;
    targets_.bind_sun_shadow(deferred_, sun_shadows_);
    for (uint32_t i = 0; i < frame.image_count(); ++i)
        if (!deferred_.water_set(i, water_targets_.refraction(i), water_targets_.reflection(i),
                targets_.sampled_depth_view(i), textures_.sampler(), water_targets_.uniform(i), error)) return false;
    if (!create_forward_targets(error)) return false;
    if (!create_postprocess(error)) return false;

    reset_required_ = false;
    error.clear();
    return true;
}

bool VulkanGameDevice::prepare_for_reset(std::string& error)
{
    if (!window_.device())
    {
        error = "Vulkan renderer reset requires a live device";
        return false;
    }
    if (!window_.frame().wait_idle())
    {
        error = "could not wait for Vulkan device before renderer reset";
        return false;
    }

    // A reset starts between frames. Drop commands accumulated for a frame
    // that will no longer be submitted, while keeping device-owned UI buffers.
    model_draws_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
    ui_.setup_states();
    error.clear();
    return true;
}

void VulkanGameDevice::destroy()
{
    if (window_.device() && frame_dispatch_.device_wait_idle)
        frame_dispatch_.device_wait_idle(window_.device());
    ui_.DestroyUIGeom();
    screenshot_buffer_.destroy();
    screenshot_requested_ = screenshot_ready_ = readback_enabled_ = false;
    model_draws_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
    scene_visibility_ = false;
    postprocess_passes_.clear();
    for (auto view : color_map_views_) if (view) textures_.release_environment(view);
    color_map_views_[0] = color_map_views_[1] = VK_NULL_HANDLE;
    active_map_names_[0].clear();
    active_map_names_[1].clear();
    targets_.release_lighting(deferred_);
    deferred_.release_water_sets();
    release_forward_targets();
    release_level_glows();
    textures_.destroy();
    if (window_.device()) window_.frame().release_swapchain();
    targets_.destroy();
    water_targets_.destroy();
    for (auto& image : local_sets_)
        for (auto& set : image) deferred_.release_gbuffer(set);
    local_sets_.clear();
    local_shadows_.destroy();
    sun_shadows_.destroy();
    ui_pass_.destroy();
    deferred_.destroy();
    shader_resources_.destroy();
    window_.destroy();
    frame_dispatch_ = {};
    texture_dispatch_ = {};
    buffer_upload_ = {};
    create_sampler_ = nullptr;
    destroy_sampler_ = nullptr;
    reset_required_ = false;
}
}
