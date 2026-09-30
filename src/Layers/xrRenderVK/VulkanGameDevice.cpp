#include "xrEngine/stdafx.h"
#include "VulkanGameDevice.h"
#include "SceneShaders.h"
#include "ShaderModule.h"

#include <algorithm>

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
    if (!window_.initialize(window, extent, false, error, true)) return false;
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
        const ShaderModuleDispatch shaders{
            proc<PFN_vkCreateShaderModule>(device, get, "vkCreateShaderModule"),
            proc<PFN_vkDestroyShaderModule>(device, get, "vkDestroyShaderModule")};
        DeferredShaderFactory deferred_factory;
        if (!deferred_factory.create(device, shaders, scene_dispatch, targets_.render_pass(),
                frame.render_pass(), deferred_, error) ||
            !targets_.bind_lighting(deferred_, error)) goto failed;
        ShaderModule scene_vertex, scene_fragment, ui_vertex, ui_fragment;
        if (!scene_vertex.initialize(device, shaders, scene_shaders::SceneVertex,
                sizeof(scene_shaders::SceneVertex), error) ||
            !scene_fragment.initialize(device, shaders, scene_shaders::SceneFragment,
                sizeof(scene_shaders::SceneFragment), error) ||
            !ui_vertex.initialize(device, shaders, scene_shaders::UiVertex,
                sizeof(scene_shaders::UiVertex), error) ||
            !ui_fragment.initialize(device, shaders, scene_shaders::UiFragment,
                sizeof(scene_shaders::UiFragment), error) ||
            !ui_pass_.initialize(device, frame.render_pass(), scene_vertex.handle(),
                scene_fragment.handle(), ui_vertex.handle(), ui_fragment.handle(),
                scene_dispatch, error, true) ||
            !textures_.initialize(device, window_.queue(), frame.command_pool(), physical.memory,
                physical.features.textureCompressionBC, texture_dispatch_,
                create_sampler, destroy_sampler, error)) goto failed;
    }
    ui_.configure(device, physical.memory, buffer_upload_.buffer, ui_pass_);
    error.clear();
    return true;
failed:
    destroy();
    return false;
}

void VulkanGameDevice::record_ui(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    owner.ui_recorded_ = owner.ui_.record(frame, owner.ui_error_);
}

void VulkanGameDevice::queue_model(GpuModel& model, IKinematics* skeleton,
    const float (&mvp)[16])
{
    ModelDraw draw;
    draw.model = &model;
    draw.skeleton = skeleton;
    std::copy_n(mvp, 16, draw.mvp.data());
    model_draws_.push_back(draw);
}

void VulkanGameDevice::queue_level_visual(uint32_t index, const float (&mvp)[16])
{
    LevelDraw draw;
    draw.index = index;
    std::copy_n(mvp, 16, draw.mvp.data());
    level_draws_.push_back(draw);
}

void VulkanGameDevice::record_level_visuals(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    for (const auto& draw : owner.level_draws_)
    {
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (!owner.current_level_->record_visual(draw.index, frame, owner.deferred_, mvp))
        {
            owner.level_recorded_ = false;
            return;
        }
    }
}

void VulkanGameDevice::record_models(const FrameRecordingContext& frame, void* user)
{
    auto& owner = *static_cast<VulkanGameDevice*>(user);
    for (auto& draw : owner.model_draws_)
    {
        float mvp[16];
        std::copy(draw.mvp.begin(), draw.mvp.end(), mvp);
        if (draw.skeleton ?
                !draw.model->record_animated(frame, owner.deferred_, mvp,
                    *draw.skeleton, owner.model_error_) :
                !draw.model->record(frame, owner.deferred_, mvp,
                    nullptr, 0, owner.model_error_))
        {
            owner.models_recorded_ = false;
            return;
        }
    }
}

bool VulkanGameDevice::render(const GpuLevel& level, const float (&mvp)[16],
    const DeferredLight& light, FrameStatus& status, std::string& error)
{
    ui_recorded_ = true;
    models_recorded_ = true;
    level_recorded_ = true;
    current_level_ = &level;
    ui_error_.clear();
    model_error_.clear();
    if (!frame_.render(window_.frame(), targets_, level, deferred_, mvp,
            light, status, error, record_ui, this, record_models, this,
            scene_visibility_ ? record_level_visuals : nullptr, this))
    {
        if (!window_.frame().device_lost())
            reset_required_ = true;
        model_draws_.clear();
        level_draws_.clear();
        current_level_ = nullptr;
        return false;
    }
    if (status == FrameStatus::RecreateRequired)
        reset_required_ = true;
    model_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
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
    if (!ui_recorded_)
    {
        error = ui_error_;
        return false;
    }
    // The per-frame GPU buffers own the submitted vertices. Clear the CPU
    // command list so the next game frame cannot replay stale UI primitives.
    ui_.reset_frame();
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

    targets_.release_lighting(deferred_);
    targets_.destroy();
    const bool replace_surface = recreate_surface || window_.frame().surface_lost();
    if (!(replace_surface ? window_.recreate_surface(extent, error) :
              window_.recreate_frame(extent, error)))
        return false;

    auto& frame = window_.frame();
    const auto& physical = window_.physical();
    if (!targets_.initialize(physical.handle, window_.device(), frame.extent(),
            static_cast<uint32_t>(frame.image_count()), frame.depth_format(), physical.memory,
            frame_dispatch_, create_sampler_, destroy_sampler_, error))
        return false;
    deferred_.rebind_compatible_render_passes(targets_.render_pass(), frame.render_pass());
    ui_pass_.rebind_render_pass(frame.render_pass());
    if (!targets_.bind_lighting(deferred_, error))
        return false;

    reset_required_ = false;
    error.clear();
    return true;
}

void VulkanGameDevice::destroy()
{
    if (window_.device() && frame_dispatch_.device_wait_idle)
        frame_dispatch_.device_wait_idle(window_.device());
    ui_.DestroyUIGeom();
    model_draws_.clear();
    level_draws_.clear();
    current_level_ = nullptr;
    scene_visibility_ = false;
    textures_.destroy();
    ui_pass_.destroy();
    targets_.release_lighting(deferred_);
    targets_.destroy();
    deferred_.destroy();
    window_.destroy();
    frame_dispatch_ = {};
    texture_dispatch_ = {};
    buffer_upload_ = {};
    create_sampler_ = nullptr;
    destroy_sampler_ = nullptr;
    reset_required_ = false;
}
}
