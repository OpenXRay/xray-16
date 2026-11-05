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

template <typename T> T proc(VkDevice device, PFN_vkGetDeviceProcAddr get, const char *name)
{
    return reinterpret_cast<T>(get(device, name));
}

#if defined(XR_PLATFORM_ANDROID)
bool set_android_buffer_extent(SDL_Window *window, VkExtent2D extent, std::string &error)
{
    SDL_SysWMinfo info{};
    SDL_VERSION(&info.version);
    if (!window || !SDL_GetWindowWMInfo(window, &info) || info.subsystem != SDL_SYSWM_ANDROID ||
        !info.info.android.window)
    {
        error = "Android Vulkan window has no native surface";
        return false;
    }
    if (ANativeWindow_setBuffersGeometry(info.info.android.window, static_cast<int32_t>(extent.width),
                                         static_cast<int32_t>(extent.height), 0) != 0)
    {
        error = "could not set Android Vulkan buffer resolution";
        return false;
    }
    return true;
}
#endif

} // namespace

void VulkanGameDevice::collect_retired(uint32_t slot)
{
    textures_.set_retirement_safe(true);
    retired_.complete(slot);
    textures_.set_retirement_safe(false);
}
bool VulkanGameDevice::wait_idle()
{
    if (window_.device() && !window_.frame().device_lost() && !window_.frame().wait_idle() &&
        !window_.frame().device_lost())
        return false;
    textures_.set_retirement_safe(true);
    retired_.drain();
    textures_.set_retirement_safe(false);
    return true;
}

bool VulkanGameDevice::initialize(SDL_Window *window, VkExtent2D extent, std::string &error)
{
    destroy();
#if defined(XR_PLATFORM_ANDROID)
    if (!extent.width || !extent.height || !set_android_buffer_extent(window, extent, error))
        return false;
    scaled_android_window_ = window;
    requested_android_extent_ = extent;
#endif
    readback_enabled_ = window_.initialize(window, extent, true, error, true, true, true);
    if (!readback_enabled_)
    {
        destroy();
        return false;
    }
    const VkDevice device = window_.device();
    const auto get = window_.device_proc();
    const auto &physical = window_.physical();
    const auto &frame = window_.frame();
#if defined(XR_PLATFORM_ANDROID)
    Msg("[renderer-vulkan] internal resolution requested=%ux%u swapchain=%ux%u", extent.width, extent.height,
        frame.extent().width, frame.extent().height);
    if (frame.extent().width != extent.width || frame.extent().height != extent.height)
        Msg("! [renderer-vulkan] Android surface did not adopt the requested buffer resolution");
#endif
    Msg("[renderer-vulkan] device.gpu name='%s' driver=0x%x api=%u.%u push-constant-limit=%u",
        physical.properties.deviceName, physical.properties.driverVersion,
        VK_VERSION_MAJOR(physical.properties.apiVersion), VK_VERSION_MINOR(physical.properties.apiVersion),
        physical.properties.limits.maxPushConstantsSize);
    if (!load_frame_dispatch(window_.instance(), window_.instance_proc(), device, get, frame_dispatch_, error))
        goto failed;

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
    texture_dispatch_.fence_after_copy = physical.properties.vendorID == 0x5143; // Qualcomm

    buffer_upload_.buffer = {texture_dispatch_.create_buffer,
                             texture_dispatch_.destroy_buffer,
                             texture_dispatch_.get_buffer_memory_requirements,
                             texture_dispatch_.allocate_memory,
                             texture_dispatch_.free_memory,
                             texture_dispatch_.bind_buffer_memory,
                             texture_dispatch_.map_memory,
                             texture_dispatch_.unmap_memory};
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
        if (!load_scene_pass_dispatch(device, get, scene_dispatch, error) || !create_sampler || !destroy_sampler ||
            !buffer_upload_.cmd_copy_buffer)
        {
            if (error.empty())
                error = "Vulkan game procedures are incomplete";
            goto failed;
        }
        std::vector<uint8_t> cached;
        string_path cache_path;
        if (FS.path_exist("$app_data_root$") && FS.exist(cache_path, "$app_data_root$", "vulkan-pipelines.cache"))
            if (IReader *reader = FS.r_open(cache_path))
            {
                if (reader->length() <= PipelineCache::MaximumBytes + 24)
                    cached.assign(static_cast<const uint8_t *>(reader->pointer()),
                                  static_cast<const uint8_t *>(reader->pointer()) + reader->length());
                FS.r_close(reader);
            }
        if (pipeline_cache_.initialize(device, get, physical.properties, cached))
        {
            scene_dispatch.pipeline_cache = pipeline_cache_.handle();
            Msg("[renderer-vulkan] pipeline.cache loaded-bytes=%zu", pipeline_cache_.loaded_bytes());
        }
        if (!targets_.initialize(physical.handle, device, frame.extent(), static_cast<uint32_t>(frame.image_count()),
                                 frame.depth_format(), physical.memory, frame_dispatch_, create_sampler,
                                 destroy_sampler, error))
            goto failed;
        if (!sun_shadows_.initialize(device, frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                                     physical.memory, frame_dispatch_, buffer_upload_.buffer, create_sampler,
                                     destroy_sampler, error))
            goto failed;
        if (!local_shadows_.initialize(device, frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                                       physical.memory, frame_dispatch_, buffer_upload_.buffer, create_sampler,
                                       destroy_sampler, error,
                                       physical.properties.limits.minUniformBufferOffsetAlignment))
            goto failed;
        local_sets_.resize(frame.image_count());
        {
            std::vector<VkImageView> depth_views;
            for (uint32_t i = 0; i < frame.image_count(); ++i)
                depth_views.push_back(targets_.depth_view(i));
            if (!window_.frame().attach_scene_depth(depth_views, error))
                goto failed;
        }
        if (!window_.frame().enable_interpass(error) ||
            !water_targets_.initialize(physical.handle, device, frame.format(), frame.extent(),
                                       static_cast<uint32_t>(frame.image_count()), physical.memory, frame_dispatch_,
                                       buffer_upload_.buffer, error))
            goto failed;
        const ShaderModuleDispatch shaders{proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
                                           proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
        configure_engine_shader_resources(device, shaders, shader_resources_);
        DeferredShaderFactory deferred_factory;
        Msg("[renderer-vulkan] device.deferred begin");
        if (!deferred_factory.create(device, shaders, scene_dispatch, targets_.render_pass(), frame.render_pass(),
                                     deferred_, shader_resources_, error, sun_shadows_.render_pass(),
                                     local_shadows_.render_pass()) ||
            !targets_.bind_lighting(deferred_, error))
            goto failed;
        // enable_interpass() creates a second, compatible render pass. Register it
        // on first startup as well as on swapchain recreation: transparent
        // geometry and UI are recorded there when the level uses water.
        deferred_.rebind_compatible_render_passes(targets_.render_pass(), frame.render_pass(),
                                                  sun_shadows_.render_pass(), local_shadows_.render_pass(),
                                                  frame.overlay_render_pass());
        if ((physical.graphics_queue_flags & VK_QUEUE_COMPUTE_BIT) &&
            !light_grid_.initialize(device, get, frame.render_pass(), deferred_.gbuffer_layout(), physical.memory,
                                    buffer_upload_.buffer, scene_dispatch, shaders,
                                    frame_dispatch_.cmd_pipeline_barrier, error))
        {
            Msg("! [renderer-vulkan] light grid unavailable; using individual lights: %s", error.c_str());
            error.clear();
        }
        Msg("[renderer-vulkan] device.deferred ready");
        targets_.bind_sun_shadow(deferred_, sun_shadows_);
        if (!create_forward_targets(error))
            goto failed;
        ShaderModule scene_vertex, scene_fragment, ui_vertex, ui_fragment;
        if (!scene_vertex.initialize(device, shaders, scene_shaders::SceneVertex, sizeof(scene_shaders::SceneVertex), error) ||
            !scene_fragment.initialize(device, shaders, scene_shaders::SceneFragment, sizeof(scene_shaders::SceneFragment), error) ||
            !ui_vertex.initialize(device, shaders, scene_shaders::UiVertex, sizeof(scene_shaders::UiVertex), error) ||
            !ui_fragment.initialize(device, shaders, scene_shaders::UiFragment, sizeof(scene_shaders::UiFragment), error) ||
            !ui_pass_.initialize(device, frame.render_pass(), scene_vertex.handle(), scene_fragment.handle(), ui_vertex.handle(), ui_fragment.handle(),
                                 scene_dispatch, error, true) ||
            !textures_.initialize(device, window_.queue(), frame.command_pool(), physical.memory, physical.features.textureCompressionBC,
                physical.features, physical.properties,
                strstr(Core.Params, "-vk-anisotropy ") ? [&]() {
                    unsigned value = 4;
                    sscanf(strstr(Core.Params, "-vk-anisotropy ") +
                        sizeof("-vk-anisotropy ") - 1, "%u", &value);
                    return float(std::clamp(value, 1u, 16u));
                }() : strstr(Core.Params, "-android-mobile-preset Minimum") ? 2.f :
                strstr(Core.Params, "-android-mobile-preset Low") ? 4.f :
                strstr(Core.Params, "-android-mobile-preset Extreme") ||
                strstr(Core.Params, "-android-mobile-preset High") ? 16.f : 8.f,
                texture_dispatch_,
                                  create_sampler, destroy_sampler, frame_dispatch_.device_wait_idle, error) ||
            !create_postprocess(error))
            goto failed;
        for (uint32_t i = 0; i < frame.image_count(); ++i)
            if (!deferred_.water_set(i, water_targets_.refraction(i), water_targets_.distortion_field(i),
                                     targets_.sampled_depth_view(i), textures_.sampler(), targets_.sampler(),
                                     water_targets_.uniform(i), error))
                goto failed;
        ui_pass_.rebind_render_pass(frame.render_pass(), frame.overlay_render_pass());
        if (!create_ui_scene_sets(error))
            goto failed;
        Msg("[renderer-vulkan] device.passes ready water-overlay=%d images=%zu",
            frame.overlay_render_pass() != VK_NULL_HANDLE, frame.image_count());
    }
    deferred_.set_game_pipeline_request(
        [this](const std::string &vs, const std::string &ps, SurfaceMode mode, bool hud, bool skinned,
               std::string &reason) { return request_shader_pair(vs, ps, mode, hud, skinned, reason); });
    ui_.configure(
        device, physical.memory, buffer_upload_.buffer, ui_pass_,
        {[this](VkDescriptorSet set) { return textures_.retain_ui(set, ui_pass_); },
         [this](VkDescriptorSet set) { retired_.defer([this, set] { textures_.release_ui(set, ui_pass_); }); }});
    deferred_.set_frame_ready([this](uint32_t slot) { collect_retired(slot); });
    error.clear();
    return true;
failed:
    Msg("! [renderer-vulkan] device.initialize failed: %s", error.c_str());
    destroy();
    return false;
}

bool VulkanGameDevice::reload_game_shaders(std::string &error)
{
    const VkDevice device = window_.device();
    if (!device || !wait_idle())
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

bool VulkanGameDevice::create_ui_scene_sets(std::string &error)
{
    ui_scene_sets_.resize(window_.frame().image_count());
    ui_depth_sets_.resize(window_.frame().image_count());
    for (uint32_t i = 0; i < ui_scene_sets_.size(); ++i)
        if (!ui_pass_.create_ui_texture_set(water_targets_.refraction(i), targets_.sampler(), ui_scene_sets_[i],
                                            error) ||
            !ui_pass_.create_ui_texture_set(targets_.sampled_depth_view(i), targets_.sampler(), ui_depth_sets_[i],
                                            error, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL))
            return false;
    return true;
}

void VulkanGameDevice::release_ui_scene_sets()
{
    for (auto &set : ui_scene_sets_)
        ui_pass_.release_ui_texture_set(set);
    ui_scene_sets_.clear();
    for (auto &set : ui_depth_sets_)
        ui_pass_.release_ui_texture_set(set);
    ui_depth_sets_.clear();
}

bool VulkanGameDevice::create_forward_targets(std::string &error)
{
    release_forward_targets();
    const auto &frame = window_.frame();
    forward_buffers_.resize(frame.image_count());
    for (uint32_t i = 0; i < frame.image_count(); ++i)
        if (!forward_buffers_[i].initialize(window_.device(), sizeof(ForwardLightUniform),
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                            window_.physical().memory, buffer_upload_.buffer, error) ||
            !deferred_.forward_set(i, forward_buffers_[i].handle(), error))
        {
            release_forward_targets();
            return false;
        }
    return true;
}

void VulkanGameDevice::release_forward_targets()
{
    deferred_.release_forward_sets();
    forward_buffers_.clear();
}

bool VulkanGameDevice::create_postprocess(std::string &error)
{
    postprocess_passes_.clear();
    auto &frame = window_.frame();
    const auto device = window_.device();
    const auto get = window_.device_proc();
    ScreenCopyDispatch dispatch;
    ShaderModuleDispatch modules{proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
                                 proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
    ShaderModule vertex, fragment;
    if (!load_screen_copy_dispatch(device, get, dispatch, error) || !dispatch.cmd_push_constants ||
        !vertex.initialize(device, modules, postprocess_shaders::Vertex, sizeof(postprocess_shaders::Vertex), error) ||
        !fragment.initialize(device, modules, postprocess_shaders::Fragment, sizeof(postprocess_shaders::Fragment),
                             error))
    {
        if (error.empty())
            error = "Vulkan postprocess requires push constants";
        return false;
    }
    for (size_t i = 0; i < frame.image_count(); ++i)
    {
        auto pass = std::make_unique<ScreenCopyPass>();
        if (!pass->initialize(device, frame.composite_render_pass(), frame.postprocess_view(i), textures_.sampler(),
                              vertex.handle(), fragment.handle(), dispatch, error))
        {
            postprocess_passes_.clear();
            return false;
        }
        pass->set_color_maps(color_map_views_[0], color_map_views_[1]);
        postprocess_passes_.push_back(std::move(pass));
    }
    return true;
}

void VulkanGameDevice::set_postprocess(const PostProcessConstants &params, std::string first, std::string second)
{
    postprocess_params_ = params;
    postprocess_params_.grade[3] = tonemap_enabled ? tonemap_amount : 0.f;
    postprocess_params_.color_map[2] = tonemap_middlegray;
    color_map_names_[0] = std::move(first);
    color_map_names_[1] = std::move(second);
}

bool VulkanGameDevice::refresh_color_maps(std::string &error)
{
    if (color_map_names_[0] == active_map_names_[0] && color_map_names_[1] == active_map_names_[1])
        return true;
    // Descriptor writes and texture eviction must wait for previous submissions.
    if (!wait_idle())
    {
        error = "could not wait for previous frames before changing postprocess color maps";
        return false;
    }
    VkImageView replacements[2]{};
    for (size_t i = 0; i < 2; ++i)
        if (!color_map_names_[i].empty() && !textures_.environment(color_map_names_[i], replacements[i], error))
        {
            for (auto view : replacements)
                if (view)
                    textures_.release_environment(view);
            return false;
        }
    if (!textures_.finish_uploads())
    {
        for (auto view : replacements)
            if (view)
                textures_.release_environment(view);
        error = "could not upload Vulkan postprocess color maps";
        return false;
    }
    for (auto &pass : postprocess_passes_)
        pass->set_color_maps(replacements[0], replacements[1]);
    for (auto view : color_map_views_)
        if (view)
            textures_.release_environment(view);
    std::copy_n(replacements, 2, color_map_views_);
    std::copy_n(color_map_names_, 2, active_map_names_);
    return true;
}

void VulkanGameDevice::queue_model(GpuModel &model, IKinematics *skeleton, const float (&mvp)[16], bool hud,
                                   float sort_distance, const void *instance, float lod, const Fmatrix *world,
                                   const float *hemi_faces, bool shadow_only)
{
    ModelDraw draw;
    draw.model = &model;
    draw.instance = instance;
    draw.skeleton = skeleton;
    std::copy_n(mvp, 16, draw.mvp.data());
    const Fmatrix &rotation = world ? *world : Fidentity;
    std::memcpy(draw.world.data(), &rotation, sizeof(rotation));
    draw.normal_rows = {rotation.i.x, rotation.j.x, rotation.k.x, -1.f,         rotation.i.y, rotation.j.y,
                        rotation.k.y, 0.f,          rotation.i.z, rotation.j.z, rotation.k.z, 0.f};
    if (hemi_faces && std::all_of(hemi_faces, hemi_faces + 6, [](float value) { return std::isfinite(value); }))
    {
        draw.normal_rows[3] = std::clamp(hemi_faces[VulkanShaderHemiFaceOrder[0]], 0.f, 1.f);
        draw.normal_rows[7] = std::clamp(hemi_faces[VulkanShaderHemiFaceOrder[1]], 0.f, 1.f);
        draw.normal_rows[11] = std::clamp(hemi_faces[VulkanShaderHemiFaceOrder[2]], 0.f, 1.f);
        for (unsigned face = 0; face < 3; ++face)
            draw.hemi_negative[face] = std::clamp(hemi_faces[VulkanShaderHemiFaceOrder[face + 3]], 0.f, 1.f);
    }
    draw.sort_distance = std::isfinite(sort_distance) ? std::max(sort_distance, 0.f) : 0.f;
    draw.lod = lod;
    draw.hud = hud;
    if (!shadow_only)
        model_draws_.push_back(draw);
    if (!hud)
    {
        const auto range = shadow_draw_lookup_.equal_range(draw.model);
        for (auto it = range.first; it != range.second; ++it)
        {
            const auto &other = shadow_model_draws_[it->second];
            if (other.skeleton == draw.skeleton && other.world == draw.world)
                return;
        }
        shadow_draw_lookup_.emplace(draw.model, shadow_model_draws_.size());
        shadow_model_draws_.push_back(draw);
    }
}

void VulkanGameDevice::queue_particle(IRenderVisual *visual, const float (&mvp)[16], const Fvector &right,
                                      const Fvector &up, bool hud, float distance)
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

void VulkanGameDevice::discard_model_draws(const void *instance)
{
    particle_draws_.erase(std::remove_if(particle_draws_.begin(), particle_draws_.end(),
                                         [instance](const ParticleDraw &draw) { return draw.visual == instance; }),
                          particle_draws_.end());
    model_draws_.erase(std::remove_if(model_draws_.begin(), model_draws_.end(),
                                      [instance](const ModelDraw &draw) { return draw.instance == instance; }),
                       model_draws_.end());
    shadow_model_draws_.erase(std::remove_if(shadow_model_draws_.begin(), shadow_model_draws_.end(),
                                             [instance](const ModelDraw &draw) { return draw.instance == instance; }),
                              shadow_model_draws_.end());
    shadow_draw_lookup_.clear();
    for (size_t i = 0; i < shadow_model_draws_.size(); ++i)
        shadow_draw_lookup_.emplace(shadow_model_draws_[i].model, i);
    const size_t previous_level_count = level_draws_.size();
    level_draws_.erase(std::remove_if(level_draws_.begin(), level_draws_.end(),
                                      [instance](const LevelDraw &draw) { return draw.instance == instance; }),
                       level_draws_.end());
    if (level_draws_.size() == previous_level_count)
        return;
    // Erasure shifts vector indices. Rebuild before another submission can
    // consult the deduplication table in this same frame.
    level_draw_lookup_.clear();
    for (size_t index = 0; index < level_draws_.size(); ++index)
        level_draw_lookup_.emplace(level_draws_[index].index, index);
}

void VulkanGameDevice::queue_level_visual(uint32_t index, const float (&mvp)[16], bool hud, float sort_distance,
                                          const void *instance, float lod)
{
    const auto existing = level_draw_lookup_.equal_range(index);
    for (auto it = existing.first; it != existing.second; ++it)
    {
        const LevelDraw &draw = level_draws_[it->second];
        if (draw.hud == hud && draw.instance == instance && std::equal(draw.mvp.begin(), draw.mvp.end(), mvp))
            return;
    }

    LevelDraw draw;
    draw.index = index;
    draw.instance = instance;
    std::copy_n(mvp, 16, draw.mvp.data());
    draw.sort_distance = std::isfinite(sort_distance) ? std::max(sort_distance, 0.f) : 0.f;
    draw.lod = lod;
    draw.hud = hud;
    level_draws_.push_back(draw);
    level_draw_lookup_.emplace(index, level_draws_.size() - 1);
}

void VulkanGameDevice::begin_frame()
{
    if (!launch_settings_applied_)
    {
        apply_vulkan_launch_settings(Core.Params);
        launch_settings_applied_ = true;
        Msg("[renderer-vulkan] launcher.settings preset=%u detail=%d ambient=%.3f hemi=%.3f sun=%.3f", quality_preset,
            detail_radius, ambient_scale, hemisphere_scale, sun_scale);
    }
    discard_scene_draws();
    light_snapshots_.clear();
    ui_.reset_frame();
    weather_set_ = VK_NULL_HANDLE;
}

VkDescriptorSet VulkanGameDevice::glow_texture(const std::string &name)
{
    if (name.empty())
        return VK_NULL_HANDLE;
    const auto found = glow_textures_.find(name);
    if (found != glow_textures_.end())
        return found->second;
    VkDescriptorSet set = VK_NULL_HANDLE;
    std::string error;
    if (!textures_.ui(name, ui_pass_, set, error))
    {
        Msg("! [renderer-vulkan] glow texture '%s': %s", name.c_str(), error.c_str());
        return VK_NULL_HANDLE;
    }
    glow_textures_.emplace(name, set);
    return set;
}

VkImageView VulkanGameDevice::light_projection(const std::string &name)
{
    if (failed_light_projections_.count(name))
        return light_projection("");
    auto found = light_projections_.find(name);
    if (found != light_projections_.end())
        return found->second;
    VkImageView view{};
    std::string error;
    if (!textures_.projection_texture(name, view, error))
    {
        Msg("! [renderer-vulkan] spot texture '%s': %s", name.c_str(), error.c_str());
        failed_light_projections_.insert(name);
        return light_projection("");
    }
    light_projections_.emplace(name, view);
    return view;
}

void VulkanGameDevice::release_level_glows()
{
    for (const auto &entry : light_projections_)
        if (!entry.first.empty() && entry.second)
            textures_.release_environment(entry.second);
    local_shadows_.invalidate_cache();
    light_projections_.clear();
    failed_light_projections_.clear();
    local_projection_views_.clear();
    local_bound_projections_.clear();

    if (glow_textures_.empty())
        return;
    R_ASSERT2(wait_idle(), "Vulkan glow descriptors require idle GPU frames");
    for (const auto &[name, set] : glow_textures_)
        textures_.release_ui(set, ui_pass_);
    glow_textures_.clear();
}

void VulkanGameDevice::queue_weather(VkDescriptorSet set, const WeatherLighting &lighting, const float (&fog_color)[3],
                                     float fog_near, float fog_far)
{
    weather_set_ = set;
    weather_lighting_ = lighting;
    std::copy_n(fog_color, 3, forward_lighting_.fog_color_start);
    forward_lighting_.fog_color_start[3] = fog_near;
    forward_lighting_.fog_end_camera[0] = fog_far;
}

void VulkanGameDevice::queue_rain(VulkanRainRender &rain)
{
    rain_draws_.push_back(&rain);
}

void VulkanGameDevice::queue_thunderbolt(VulkanThunderboltRender &bolt)
{
    thunderbolt_draws_.push_back(&bolt);
}

void VulkanGameDevice::discard_rain(const VulkanRainRender *rain)
{
    rain_draws_.erase(std::remove(rain_draws_.begin(), rain_draws_.end(), rain), rain_draws_.end());
}

void VulkanGameDevice::discard_thunderbolt(const VulkanThunderboltRender *bolt)
{
    thunderbolt_draws_.erase(std::remove(thunderbolt_draws_.begin(), thunderbolt_draws_.end(), bolt),
                             thunderbolt_draws_.end());
}

bool VulkanGameDevice::take_screenshot(std::vector<uint8_t> &pixels, VkExtent2D &extent, VkFormat &format)
{
    if (!screenshot_ready_)
        return false;
    screenshot_ready_ = false;
    extent = screenshot_extent_;
    format = window_.frame().format();
    pixels.resize(size_t(extent.width) * extent.height * 4);
    std::string error;
    if (!screenshot_buffer_.read(0, pixels.data(), pixels.size(), error))
    {
        Msg("! [renderer-vulkan] screenshot: %s", error.c_str());
        return false;
    }
    return true;
}

void VulkanGameDevice::discard_scene_draws()
{
    model_draws_.clear();
    shadow_model_draws_.clear();
    shadow_draw_lookup_.clear();
    particle_draws_.clear();
    rain_draws_.clear();
    thunderbolt_draws_.clear();
    level_draws_.clear();
    level_draw_lookup_.clear();
    current_level_ = nullptr;
}

bool VulkanGameDevice::render(const GpuLevel &level, const float (&mvp)[16], const DeferredLight &light,
                              FrameStatus &status, std::string &error, bool render_world, bool clear_target,
                              const DeferredEnvironment *environment)
{
    ui_recorded_ = true;
    models_recorded_ = true;
    local_lights_recorded_ = true;
    level_recorded_ = true;
    level_error_.clear();
    current_level_ = &level;
    std::copy(std::begin(mvp), std::end(mvp), scene_mvp_.begin());
    ui_error_.clear();
    model_error_.clear();
    if (!refresh_color_maps(error))
        return false;
    for (auto &pass : postprocess_passes_)
        pass->set_constants(postprocess_params_);
    const float weather_blend = weather_lighting_.light.color[3];
    const float cloud_params = weather_lighting_.light.grade[3];
    weather_lighting_.light = light;
    weather_lighting_.light.color[3] = weather_blend;
    weather_lighting_.light.grade[3] = cloud_params;
    last_ui_draw_calls_ = ui_.draw_calls();
    last_ui_triangles_ = ui_.triangles();
    if (Device.dwPrecacheFrame &&
        (Device.dwPrecacheFrame == Device.dwPrecacheTotal || Device.dwPrecacheFrame % 10 == 0))
        Msg("[renderer-vulkan] frame.begin precache=%u world=%d clear=%d water=%d ui-draws=%u level-draws=%zu "
            "model-draws=%zu particles=%zu ambient=%.3f sun=(%.3f,%.3f,%.3f)",
            Device.dwPrecacheFrame, render_world ? 1 : 0, clear_target ? 1 : 0, level.has_water() ? 1 : 0,
            last_ui_draw_calls_, level_draws_.size(), model_draws_.size(), particle_draws_.size(),
            light.direction_ambient[3], light.color[0], light.color[1], light.color[2]);
    screenshot_ready_ = false;
    if (screenshot_requested_ && readback_enabled_)
    {
        screenshot_extent_ = window_.frame().extent();
        const VkDeviceSize bytes = VkDeviceSize(screenshot_extent_.width) * screenshot_extent_.height * 4;
        if (screenshot_buffer_.size() < bytes)
        {
            if (!wait_idle() || !screenshot_buffer_.initialize(
                                    window_.device(), bytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                    window_.physical().memory, buffer_upload_.buffer, error))
                return false;
        }
    }
    SunShadowUniform sun_uniform{};
    if (render_world && Core.Params)
    {
        // The old argument remains accepted for existing custom launchers.
        const char *mode = strstr(Core.Params, "-vk-light-debug=");
        if (mode)
        {
            const char value = mode[16]; // one ASCII digit after the '='
            if (value >= '1' && value <= '9' && (mode[17] == '\0' || mode[17] == ' '))
                sun_uniform.options[1] = float(value - '0');
        }
        else if (strstr(Core.Params, "-vk-light-diagnostic"))
            sun_uniform.options[1] = 1.f;
    }
    for (int channel = 0; channel < 3; ++channel)
    {
        const auto valid = [](float value) { return std::isfinite(value) ? std::max(value, 0.f) : 0.f; };
        const float ambient =
            environment ? valid(environment->ambient_color[channel]) : valid(light.direction_ambient[3]);
        // Match the R2 combine constants, including the floor before the
        // default ambient scale. The raw weather ambient is half of
        // the value passed to hmodel on the desktop renderer.
        sun_uniform.ambient_color[channel] = std::max(ambient * 2.f * ambient_scale, .001f);
        sun_uniform.hemi_color[channel] = environment ? valid(environment->hemi_color[channel]) : valid(light.grade[3]);
        sun_uniform.hemi_color[channel] *= hemisphere_scale;
    }
    if (sun_uniform.options[1] > .5f && (Device.dwFrame < 3 || Device.dwFrame % 300 == 0))
        Msg("[renderer-vulkan] lighting.diagnostic frame=%u mode=%u "
            "panels=1:base,sun,baked,albedo;6:env,hemi-one,sun-one,both;7:old-flat shadow-size=%u bias=%.4f "
            "forward=single-base-only quad=opaque-only",
            Device.dwFrame, unsigned(sun_uniform.options[1]), SunShadowTargets::Size, sun_uniform.options[0]);
    if (render_world && Device.dwFrame % 300 == 0)
        Msg("[renderer-vulkan] lighting.rgb frame=%u ambient=(%.4f,%.4f,%.4f) hemi=(%.4f,%.4f,%.4f) "
            "sun=(%.4f,%.4f,%.4f) combine=%s diagnostic=%.0f",
            Device.dwFrame, sun_uniform.ambient_color[0], sun_uniform.ambient_color[1], sun_uniform.ambient_color[2],
            sun_uniform.hemi_color[0], sun_uniform.hemi_color[1], sun_uniform.hemi_color[2], light.color[0],
            light.color[1], light.color[2], weather_set_ ? "weather" : "plain", sun_uniform.options[1]);
    WaterSceneUniform water_uniform{};
    {
        Fmatrix camera, inverse, sun_view, sun_projection, sun_vp;
        std::memcpy(&camera, mvp, sizeof(camera));
        inverse.invert_44(camera);
        std::memcpy(sun_uniform.inverse_view_projection, &inverse, sizeof(inverse));
        std::memcpy(water_uniform.view_projection, &camera, sizeof(camera));
        std::memcpy(water_uniform.inverse_view_projection, &inverse, sizeof(inverse));
        std::memcpy(forward_lighting_.inverse_view_projection, &inverse, sizeof(inverse));
        forward_lighting_.fog_end_camera[1] = Device.vCameraPosition.x;
        forward_lighting_.fog_end_camera[2] = Device.vCameraPosition.y;
        forward_lighting_.fog_end_camera[3] = Device.vCameraPosition.z;
        water_uniform.camera_position[0] = Device.vCameraPosition.x;
        water_uniform.camera_position[1] = Device.vCameraPosition.y;
        water_uniform.camera_position[2] = Device.vCameraPosition.z;
        water_uniform.camera_position[3] = 1.f;
        Fvector direction;
        direction.set(light.direction_ambient[0], light.direction_ambient[1], light.direction_ambient[2]);
        if (direction.square_magnitude() < 0.0001f)
            direction.set(0.f, -1.f, 0.f);
        direction.normalize_safe();
        Fvector up;
        up.set(0.f, 1.f, 0.f);
        if (std::abs(direction.y) > .95f)
            up.set(0.f, 0.f, 1.f);
        Fvector origin;
        origin.mad(Device.vCameraPosition, direction, -140.f);
        sun_view.build_camera_dir(origin, direction, up);
        const float shadow_span = sun_shadow_span(quality_preset);
        sun_projection.build_projection_ortho(shadow_span, shadow_span, 1.f, 320.f);
        sun_uniform.options[2] = .08f;
#if defined(XR_PLATFORM_ANDROID)
        if (Device.dwFrame % 300 == 0)
            Msg("[renderer-vulkan] shadow.range frame=%u preset=%u span=%.1fm texel=%.4fm", Device.dwFrame,
                quality_preset, shadow_span, shadow_span / SunShadowTargets::Size);
#endif
        sun_vp.mul(sun_projection, sun_view);
        stabilize_shadow_projection(reinterpret_cast<float *>(&sun_vp), SunShadowTargets::Size);
        std::memcpy(sun_uniform.sun_view_projection, &sun_vp, sizeof(sun_vp));
    }
    render_lights(mvp, render_world, light, sun_uniform);
    deferred_.set_forward_weather(render_world ? weather_set_ : VK_NULL_HANDLE);
    if (render_world && Device.dwFrame % 300 == 0)
        Msg("[renderer-vulkan] shadow.casters visible-models=%zu caster-models=%zu", model_draws_.size(),
            shadow_model_draws_.size());
    frame_.set_shadow_recorder(record_shadow_models, this);
    frame_.set_light_grid_recorder(prepare_light_grid, this);
    bool particle_refraction = false;
    bool world_refraction = false;
    for (const auto &particle : particle_draws_)
    {
        const bool active = particle.visual->getType() == 8
            ? static_cast<VulkanParticleEffect *>(particle.visual)->has_active_distortion()
            : static_cast<VulkanParticleGroup *>(particle.visual)->has_distortion();
        particle_refraction |= active;
        world_refraction |= active && !particle.hud;
        if (world_refraction)
            break;
    }
    frame_.set_distortion_recorder(render_world && world_refraction ? record_distortion : nullptr, this);
    // All gameplay texture requests for this frame have now been gathered.
    // The completed copy fences provide the Adreno transfer visibility before
    // any rendering command can sample a newly loaded image.
    if (!textures_.finish_uploads())
    {
        error = "Vulkan gameplay texture batch did not complete";
        reset_required_ = true;
        return false;
    }
    if (!frame_.render(window_.frame(), targets_, level, deferred_, mvp, light, status, error, record_ui, this,
                       record_hud, this, record_models, this, record_transparent, this,
                       scene_visibility_ ? record_level_visuals : nullptr, this, render_world, clear_target,
                       render_world ? weather_set_ : VK_NULL_HANDLE, &weather_lighting_,
                       screenshot_requested_ && readback_enabled_ ? record_readback : nullptr, this, record_postprocess,
                       this, &sun_shadows_, &sun_uniform, &local_shadows_, &local_uniforms_, record_local_lights, this,
                       render_world && (level.has_water() || particle_refraction) ? &water_targets_ : nullptr,
                       &water_uniform, ui_.has_distortion() ? capture_ui : nullptr, this,
                       ui_.has_distortion() ? record_ui_distortion : nullptr, this))
    {
        if (!window_.frame().device_lost())
            reset_required_ = true;
        model_draws_.clear();
        shadow_model_draws_.clear();
        shadow_draw_lookup_.clear();
        particle_draws_.clear();
        level_draws_.clear();
        level_draw_lookup_.clear();
        current_level_ = nullptr;
        ui_.reset_frame();
        return false;
    }
    if (Device.dwFrame % 300 == 0)
    {
        const auto &timing = window_.frame().cpu_timing();
        Msg("[renderer-vulkan] frame.cpu frame=%u fence=%.3f acquire=%.3f record=%.3f submit=%.3f present=%.3f",
            Device.dwFrame, timing.fence_ms, timing.acquire_ms, timing.record_ms, timing.submit_ms, timing.present_ms);
    }
    if (status == FrameStatus::RecreateRequired)
        reset_required_ = true;
    else if (screenshot_requested_ && readback_enabled_)
        screenshot_ready_ = wait_idle();
    screenshot_requested_ = false;
    if (Device.dwPrecacheFrame == 1)
        save_pipeline_cache();
    model_draws_.clear();
    shadow_model_draws_.clear();
    shadow_draw_lookup_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    level_draw_lookup_.clear();
    current_level_ = nullptr;
    // A successful submission consumes every CPU-side command, even when an
    // individual recorder reported malformed input after recording began.
    ui_.reset_frame();
    if (!level_recorded_)
    {
        error = level_error_.empty() ? "Vulkan level visual recording failed" : level_error_;
        return false;
    }
    if (!models_recorded_)
    {
        error = model_error_.empty() ? "Vulkan model/shadow recording failed" : model_error_;
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

bool VulkanGameDevice::recreate_swapchain(VkExtent2D extent, std::string &error, bool recreate_surface)
{
    if (!window_.device() || !extent.width || !extent.height)
    {
        error = "Vulkan swapchain reset requires a live device and nonzero drawable extent";
        return false;
    }
    if (!wait_idle())
    {
        error = "could not wait for Vulkan device before renderer reset";
        return false;
    }

    const bool replace_surface = recreate_surface || window_.frame().surface_lost();
    targets_.release_lighting(deferred_);
    deferred_.release_water_sets();
    release_ui_scene_sets();
    for (auto &image : local_sets_)
        for (auto &set : image)
            deferred_.release_gbuffer(set);
    local_sets_.clear();
    local_descriptor_buffers_.clear();
    local_bound_projections_.clear();
    release_forward_targets();
    postprocess_passes_.clear();
    if (!window_.frame().release_swapchain())
    {
        error = "could not release Vulkan framebuffers before scene depth";
        return false;
    }
    targets_.destroy();
    water_targets_.destroy();
#if defined(XR_PLATFORM_ANDROID)
    if (!set_android_buffer_extent(scaled_android_window_, requested_android_extent_, error))
        return false;
    extent = requested_android_extent_;
#endif
    if (!(replace_surface ? window_.recreate_surface(extent, error) : window_.recreate_frame(extent, error)))
        return false;

    auto &frame = window_.frame();
    const auto &physical = window_.physical();
#if defined(XR_PLATFORM_ANDROID)
    Msg("[renderer-vulkan] internal resolution reset requested=%ux%u swapchain=%ux%u", requested_android_extent_.width,
        requested_android_extent_.height, frame.extent().width, frame.extent().height);
#endif
    if (!targets_.initialize(physical.handle, window_.device(), frame.extent(),
                             static_cast<uint32_t>(frame.image_count()), frame.depth_format(), physical.memory,
                             frame_dispatch_, create_sampler_, destroy_sampler_, error))
        return false;
    sun_shadows_.destroy();
    if (!sun_shadows_.initialize(window_.device(), frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                                 physical.memory, frame_dispatch_, buffer_upload_.buffer, create_sampler_,
                                 destroy_sampler_, error))
        return false;
    local_shadows_.destroy();
    if (!local_shadows_.initialize(window_.device(), frame.depth_format(), static_cast<uint32_t>(frame.image_count()),
                                   physical.memory, frame_dispatch_, buffer_upload_.buffer, create_sampler_,
                                   destroy_sampler_, error, physical.properties.limits.minUniformBufferOffsetAlignment))
        return false;
    local_sets_.resize(frame.image_count());
    {
        std::vector<VkImageView> depth_views;
        for (uint32_t i = 0; i < frame.image_count(); ++i)
            depth_views.push_back(targets_.depth_view(i));
        if (!frame.attach_scene_depth(depth_views, error))
            return false;
    }
    if (!frame.enable_interpass(error) ||
        !water_targets_.initialize(physical.handle, window_.device(), frame.format(), frame.extent(),
                                   static_cast<uint32_t>(frame.image_count()), physical.memory, frame_dispatch_,
                                   buffer_upload_.buffer, error))
        return false;
    deferred_.rebind_compatible_render_passes(targets_.render_pass(), frame.render_pass(), sun_shadows_.render_pass(),
                                              local_shadows_.render_pass(), frame.overlay_render_pass());
    ui_pass_.rebind_render_pass(frame.render_pass(), frame.overlay_render_pass());
    if (!create_ui_scene_sets(error))
        return false;
    if (!targets_.bind_lighting(deferred_, error))
        return false;
    targets_.bind_sun_shadow(deferred_, sun_shadows_);
    for (uint32_t i = 0; i < frame.image_count(); ++i)
        if (!deferred_.water_set(i, water_targets_.refraction(i), water_targets_.distortion_field(i),
                                 targets_.sampled_depth_view(i), textures_.sampler(), targets_.sampler(),
                                 water_targets_.uniform(i), error))
            return false;
    if (!create_forward_targets(error))
        return false;
    if (!create_postprocess(error))
        return false;

    reset_required_ = false;
    error.clear();
    return true;
}

bool VulkanGameDevice::prepare_for_reset(std::string &error)
{
    if (!window_.device())
    {
        error = "Vulkan renderer reset requires a live device";
        return false;
    }
    if (!wait_idle())
    {
        error = "could not wait for Vulkan device before renderer reset";
        return false;
    }

    // A reset starts between frames. Drop commands accumulated for a frame
    // that will no longer be submitted, while keeping device-owned UI buffers.
    model_draws_.clear();
    shadow_model_draws_.clear();
    shadow_draw_lookup_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    level_draw_lookup_.clear();
    current_level_ = nullptr;
    ui_.setup_states();
    error.clear();
    return true;
}

void VulkanGameDevice::save_pipeline_cache()
{
    if (!pipeline_cache_.handle() || !FS.path_exist("$app_data_root$"))
        return;
    const auto bytes = pipeline_cache_.snapshot();
    if (bytes.empty())
        return;
    string_path destination, temporary;
    FS.update_path(destination, "$app_data_root$", "vulkan-pipelines.cache");
    FS.update_path(temporary, "$app_data_root$", "vulkan-pipelines.cache.tmp");
    if (IWriter *writer = FS.w_open(temporary))
    {
        writer->w(bytes.data(), bytes.size());
        FS.w_close(writer);
        FS.file_rename(temporary, destination, true);
        Msg("[renderer-vulkan] pipeline.cache saved-bytes=%zu", bytes.size());
    }
}

void VulkanGameDevice::destroy()
{
    launch_settings_applied_ = false;
    Msg("[renderer-vulkan] teardown game device begin");
    if (window_.device())
        R_ASSERT2(wait_idle(), "Vulkan shutdown requires completed frames");
    Msg("[renderer-vulkan] teardown game device idle");
    save_pipeline_cache();
    ui_.DestroyUIGeom();
    R_ASSERT2(wait_idle(), "Vulkan UI shutdown requires completed descriptor leases");
    screenshot_buffer_.destroy();
    screenshot_requested_ = screenshot_ready_ = readback_enabled_ = false;
    model_draws_.clear();
    shadow_model_draws_.clear();
    shadow_draw_lookup_.clear();
    particle_draws_.clear();
    level_draws_.clear();
    level_draw_lookup_.clear();
    current_level_ = nullptr;
    scene_visibility_ = false;
    postprocess_passes_.clear();
    for (auto view : color_map_views_)
        if (view)
            textures_.release_environment(view);
    color_map_views_[0] = color_map_views_[1] = VK_NULL_HANDLE;
    active_map_names_[0].clear();
    active_map_names_[1].clear();
    targets_.release_lighting(deferred_);
    deferred_.release_water_sets();
    release_ui_scene_sets();
    release_forward_targets();
    release_level_glows();
    Msg("[renderer-vulkan] teardown textures begin");
    textures_.destroy();
    Msg("[renderer-vulkan] teardown textures end");
    if (window_.device())
        window_.frame().release_swapchain();
    targets_.destroy();
    water_targets_.destroy();
    for (auto &image : local_sets_)
        for (auto &set : image)
            deferred_.release_gbuffer(set);
    local_sets_.clear();
    local_descriptor_buffers_.clear();
    local_bound_projections_.clear();
    local_shadows_.destroy();
    sun_shadows_.destroy();
    ui_pass_.destroy();
    light_grid_.destroy();
    deferred_.destroy();
    pipeline_cache_.destroy();
    shader_resources_.destroy();
    Msg("[renderer-vulkan] teardown window begin");
    window_.destroy();
#if defined(XR_PLATFORM_ANDROID)
    if (scaled_android_window_)
    {
        std::string ignored;
        if (!set_android_buffer_extent(scaled_android_window_, {}, ignored))
            Msg("! [renderer-vulkan] restore Android buffer geometry: %s", ignored.c_str());
        scaled_android_window_ = nullptr;
        requested_android_extent_ = {};
    }
#endif
    Msg("[renderer-vulkan] teardown window end");
    frame_dispatch_ = {};
    texture_dispatch_ = {};
    buffer_upload_ = {};
    create_sampler_ = nullptr;
    destroy_sampler_ = nullptr;
    reset_required_ = false;
}
} // namespace xray::render::vulkan
