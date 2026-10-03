#include "stdafx.h"

#if defined(XR_PLATFORM_ANDROID)

#include "android_vulkan_smoke.h"
#include "../Layers/xrRenderVK/FrameContext.h"
#include "../Layers/xrRenderVK/BufferResource.h"
#include "../Layers/xrRenderVK/EngineTextureSource.h"
#include "../Layers/xrRenderVK/DeferredPass.h"
#include "../Layers/xrRenderVK/DeferredShaderFactory.h"
#include "../Layers/xrRenderVK/GBufferTargets.h"
#include "../Layers/xrRenderVK/ScreenCopyPass.h"
#include "../Layers/xrRenderVK/ScenePass.h"
#include "../Layers/xrRenderVK/SceneShaders.h"
#include "../Layers/xrRenderVK/ShaderModule.h"
#include "../Layers/xrRenderVK/SmokeShaders.h"
#include "../Layers/xrRenderVK/SmokeTrianglePass.h"
#include "../Layers/xrRenderVK/VulkanHardware.h"
#include "../Layers/xrRenderVK/VulkanWindowDevice.h"

#include <SDL.h>

#if __has_include(<vulkan/vulkan.h>)
#define XRAY_ANDROID_HAS_VULKAN_HEADERS 1
#include <vulkan/vulkan.h>
#endif

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#ifndef SDL_WINDOW_VULKAN
#define SDL_WINDOW_VULKAN 0x10000000u
#endif

namespace AndroidVulkanSmoke
{
#if defined(XRAY_ANDROID_HAS_VULKAN_HEADERS)
namespace
{
struct FrameCallbackAudit
{
    uint32_t calls = 0;
    bool valid = true;
    const xray::render::vulkan::ScreenCopyPass* screen_copy = nullptr;
    const xray::render::vulkan::SmokeTrianglePass* triangle = nullptr;
    const xray::render::vulkan::ScenePass* scene = nullptr;
    VkBuffer scene_vertices = VK_NULL_HANDLE;
    VkBuffer scene_indices = VK_NULL_HANDLE;
    VkBuffer ui_vertices = VK_NULL_HANDLE;
    VkBuffer ui_indices = VK_NULL_HANDLE;
    VkDescriptorSet ui_texture_set = VK_NULL_HANDLE;
    xray::render::vulkan::SceneConstants scene_constants{};
};

struct PixelReadback
{
    VkBuffer buffer = VK_NULL_HANDLE;
    PFN_vkCmdCopyImageToBuffer copy{};
    PFN_vkCmdPipelineBarrier barrier{};
    uint32_t copies = 0;
};

struct DeferredAudit
{
    xray::render::vulkan::GBufferTargets* targets{};
    xray::render::vulkan::DeferredPass* pass{};
    xray::render::vulkan::ScenePass* ui{};
    VkBuffer vertices{}, indices{}, ui_vertices{}, ui_indices{};
    VkDescriptorSet material{}, ui_texture{};
    float mvp[16]{};
    xray::render::vulkan::DeferredLight light{};
    uint32_t geometry_calls{}, lighting_calls{};
    bool valid{true};
};

void record_deferred_geometry(const xray::render::vulkan::FrameRecordingContext& frame, void* user)
{
    auto& audit = *static_cast<DeferredAudit*>(user);
    xray::render::vulkan::FrameRecordingContext geometry;
    if (!audit.targets->begin(frame, geometry)) { audit.valid = false; return; }
    audit.valid = audit.pass->record_geometry(geometry, audit.vertices,
        audit.indices, 3, audit.mvp, audit.material) && audit.valid;
    audit.targets->end(frame.command_buffer);
    ++audit.geometry_calls;
}

void record_deferred_lighting(const xray::render::vulkan::FrameRecordingContext& frame, void* user)
{
    auto& audit = *static_cast<DeferredAudit*>(user);
    audit.valid = audit.pass->record_lighting(frame,
        audit.targets->lighting_set(frame.image_index), audit.light) && audit.valid;
    audit.valid = audit.ui->record_ui(frame, audit.ui_vertices, audit.ui_indices,
        VK_INDEX_TYPE_UINT16, 6, audit.ui_texture) && audit.valid;
    ++audit.lighting_calls;
}

void record_pixel_readback(VkCommandBuffer command, VkImage image, VkExtent2D extent, void* user_data)
{
    auto& readback = *static_cast<PixelReadback*>(user_data);
    VkBufferImageCopy regions[2]{};
    for (auto& region : regions)
    {
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {1, 1, 1};
    }
    regions[0].imageOffset = {static_cast<int32_t>(extent.width / 2),
        static_cast<int32_t>(extent.height / 2), 0};
    regions[1].bufferOffset = 4;
    regions[1].imageOffset = {60, 40, 0};
    readback.copy(command, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 2, regions);
    VkBufferMemoryBarrier host_read{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    host_read.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    host_read.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    host_read.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    host_read.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    host_read.buffer = readback.buffer;
    host_read.size = 8;
    readback.barrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
        0, 0, nullptr, 1, &host_read, 0, nullptr);
    ++readback.copies;
}

void audit_frame_callback(const xray::render::vulkan::FrameRecordingContext& frame, void* user_data)
{
    auto& audit = *static_cast<FrameCallbackAudit*>(user_data);
    audit.valid = audit.valid && frame.command_buffer && frame.render_pass && frame.framebuffer &&
        frame.extent.width && frame.extent.height &&
        frame.frame_index < xray::render::vulkan::FrameContext::FramesInFlight;
    if (audit.screen_copy)
        audit.screen_copy->record(frame);
    else if (audit.triangle)
        audit.triangle->record(frame);
    if (audit.scene)
    {
        audit.valid = audit.scene->record_geometry(frame, audit.scene_vertices, audit.scene_indices,
            VK_INDEX_TYPE_UINT16, 3, audit.scene_constants) && audit.valid;
        audit.valid = audit.scene->record_ui(frame, audit.ui_vertices, audit.ui_indices,
            VK_INDEX_TYPE_UINT16, 6, audit.ui_texture_set) && audit.valid;
    }
    ++audit.calls;
}

bool load_shader_from_vfs(pcstr name, VkDevice device,
    const xray::render::vulkan::ShaderModuleDispatch& dispatch,
    xray::render::vulkan::ShaderModule& module, std::string& error)
{
    string_path path;
    if (!FS.exist(path, "$game_shaders$", name, ".spv"))
    {
        error = std::string("compiled Vulkan shader is missing: ") + name;
        return false;
    }
    IReader* reader = FS.r_open(path);
    if (!reader)
    {
        error = std::string("could not read compiled Vulkan shader: ") + path;
        return false;
    }
    const bool loaded = module.initialize_bytes(device, dispatch,
        reader->pointer(), reader->length(), error);
    FS.r_close(reader);
    return loaded;
}

template <typename T>
T load_device_proc(VkDevice device, PFN_vkGetDeviceProcAddr get_proc, const char* name)
{
    return reinterpret_cast<T>(get_proc(device, name));
}


}
#endif

bool Run(std::string& reason)
{
#if !defined(XRAY_ANDROID_HAS_VULKAN_HEADERS)
    reason = "Vulkan headers are not provided by the Android toolchain";
    Msg("! [renderer-vulkan] %s", reason.c_str());
    return false;
#else
    SDL_Window* window = nullptr;
    xray::render::vulkan::VulkanWindowDevice platform;
    auto& frame_context = platform.frame();
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    xray::render::vulkan::TextureUploadDispatch texture_dispatch{};
    xray::render::vulkan::UploadedTexture engine_texture{};
    xray::render::vulkan::UploadedTexture ui_texture{};
    xray::render::vulkan::ImageStateTracker image_states;
    std::vector<xray::render::vulkan::PendingTextureUpload> texture_uploads;
    xray::render::vulkan::ShaderModule copy_vertex;
    xray::render::vulkan::ShaderModule copy_fragment;
    xray::render::vulkan::ScreenCopyPass screen_copy;
    xray::render::vulkan::ShaderModule smoke_vertex;
    xray::render::vulkan::ShaderModule smoke_fragment;
    xray::render::vulkan::SmokeTrianglePass smoke_triangle;
    xray::render::vulkan::ShaderModule scene_vertex, scene_fragment, ui_vertex, ui_fragment;
    xray::render::vulkan::ScenePass scene_pass;
    xray::render::vulkan::GBufferTargets deferred_targets;
    xray::render::vulkan::DeferredPass deferred_pass;
    xray::render::vulkan::BufferResource deferred_vertices, deferred_indices;
    xray::render::vulkan::BufferResource scene_vertices, scene_indices, ui_vertices, ui_indices;
    xray::render::vulkan::BufferResource pixel_buffer;
    VkSampler copy_sampler = VK_NULL_HANDLE;
    PFN_vkDestroySampler destroy_sampler = nullptr;
    PFN_vkDeviceWaitIdle wait_idle = nullptr;

    auto cleanup = [&]
    {
        if (device && wait_idle)
            wait_idle(device);
        screen_copy.destroy();
        deferred_pass.destroy();
        deferred_targets.destroy();
        deferred_vertices.destroy();
        deferred_indices.destroy();
        scene_pass.destroy();
        scene_vertex.destroy();
        scene_fragment.destroy();
        ui_vertex.destroy();
        ui_fragment.destroy();
        scene_vertices.destroy();
        scene_indices.destroy();
        ui_vertices.destroy();
        ui_indices.destroy();
        smoke_triangle.destroy();
        smoke_vertex.destroy();
        smoke_fragment.destroy();
        copy_vertex.destroy();
        copy_fragment.destroy();
        if (copy_sampler && destroy_sampler)
            destroy_sampler(device, copy_sampler, nullptr);
        pixel_buffer.destroy();
        if (!texture_uploads.empty())
            xray::render::vulkan::wait_for_uploads(device, frame_context.command_pool(),
                texture_dispatch, texture_uploads);
        if (engine_texture.image)
        {
            image_states.forget_image(engine_texture.image);
            xray::render::vulkan::destroy_texture(device, texture_dispatch, engine_texture);
        }
        if (ui_texture.image)
        {
            image_states.forget_image(ui_texture.image);
            xray::render::vulkan::destroy_texture(device, texture_dispatch, ui_texture);
        }
        platform.destroy();
        if (window)
            SDL_DestroyWindow(window);
    };

    auto fail = [&](const std::string& message)
    {
        reason = message;
        Msg("! [renderer-vulkan] %s", reason.c_str());
        cleanup();
        return false;
    };

    window = SDL_CreateWindow("OpenXRay Vulkan surface smoke", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, 960, 540, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
        return fail(SDL_GetError());
    const bool no_game_vfs = !FS.path_exist("$game_textures$");
    std::string frame_error;
    if (!platform.initialize(window, {960, 540}, no_game_vfs, frame_error, no_game_vfs))
        return fail(frame_error);
    device = platform.device();
    queue = platform.queue();
    const auto get_device_proc = platform.device_proc();
    const auto& physical_selection = platform.physical();
    const VkPhysicalDevice physical_device = physical_selection.handle;
    const uint32_t queue_family = physical_selection.graphics_present_family;
    const VkPhysicalDeviceProperties& physical_properties = physical_selection.properties;
    const VkPhysicalDeviceFeatures& physical_features = physical_selection.features;
    const VkDeviceSize device_local_bytes = physical_selection.local_memory_bytes;
    const auto get_format_properties = reinterpret_cast<PFN_vkGetPhysicalDeviceFormatProperties>(
        platform.instance_proc()(platform.instance(), "vkGetPhysicalDeviceFormatProperties"));
    const auto format_features = [&](VkFormat format)
    {
        VkFormatProperties properties{};
        if (get_format_properties)
            get_format_properties(physical_device, format, &properties);
        return properties.optimalTilingFeatures;
    };
    const bool rgba8_attachment = format_features(VK_FORMAT_R8G8B8A8_UNORM) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool rgba16f_attachment = format_features(VK_FORMAT_R16G16B16A16_SFLOAT) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool r32f_attachment = format_features(VK_FORMAT_R32_SFLOAT) &
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    const bool d24s8_attachment = format_features(VK_FORMAT_D24_UNORM_S8_UINT) &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    const bool d32s8_attachment = format_features(VK_FORMAT_D32_SFLOAT_S8_UINT) &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    Msg("[renderer-vulkan] device='%s' api=%u.%u.%u driver=0x%x vendor=0x%x device=0x%x queue=%u",
        physical_properties.deviceName,
        VK_VERSION_MAJOR(physical_properties.apiVersion), VK_VERSION_MINOR(physical_properties.apiVersion),
        VK_VERSION_PATCH(physical_properties.apiVersion), physical_properties.driverVersion,
        physical_properties.vendorID, physical_properties.deviceID, queue_family);
    Msg("[renderer-vulkan] limits: image2D=%u colorAttachments=%u samplers/stage=%u pushConstants=%u localMemory=%lluMiB",
        physical_properties.limits.maxImageDimension2D, physical_properties.limits.maxColorAttachments,
        physical_properties.limits.maxPerStageDescriptorSamplers,
        physical_properties.limits.maxPushConstantsSize,
        static_cast<unsigned long long>(device_local_bytes / (1024ull * 1024ull)));
    Msg("[renderer-vulkan] features: anisotropy=%u BC=%u ETC2=%u ASTC_LDR=%u geometry=%u tessellation=%u",
        physical_features.samplerAnisotropy, physical_features.textureCompressionBC,
        physical_features.textureCompressionETC2, physical_features.textureCompressionASTC_LDR,
        physical_features.geometryShader, physical_features.tessellationShader);
    Msg("[renderer-vulkan] attachment formats: RGBA8=%u RGBA16F=%u R32F=%u D24S8=%u D32S8=%u",
        rgba8_attachment, rgba16f_attachment, r32f_attachment, d24s8_attachment, d32s8_attachment);

    xray::render::vulkan::FrameDispatch frame_dispatch;
    if (!xray::render::vulkan::load_frame_dispatch(platform.instance(), platform.instance_proc(),
            device, get_device_proc, frame_dispatch, frame_error))
        return fail(frame_error);
    wait_idle = frame_dispatch.device_wait_idle;

    // Both game DDS resources and the no-game UI atlas use the same upload path.
#define XRAY_TEXTURE_PROC(field, name) \
            texture_dispatch.field = load_device_proc<decltype(texture_dispatch.field)>(device, get_device_proc, name)
            XRAY_TEXTURE_PROC(create_buffer, "vkCreateBuffer");
            XRAY_TEXTURE_PROC(destroy_buffer, "vkDestroyBuffer");
            XRAY_TEXTURE_PROC(get_buffer_memory_requirements, "vkGetBufferMemoryRequirements");
            XRAY_TEXTURE_PROC(create_image, "vkCreateImage");
            XRAY_TEXTURE_PROC(destroy_image, "vkDestroyImage");
            XRAY_TEXTURE_PROC(get_image_memory_requirements, "vkGetImageMemoryRequirements");
            XRAY_TEXTURE_PROC(allocate_memory, "vkAllocateMemory");
            XRAY_TEXTURE_PROC(free_memory, "vkFreeMemory");
            XRAY_TEXTURE_PROC(bind_buffer_memory, "vkBindBufferMemory");
            XRAY_TEXTURE_PROC(bind_image_memory, "vkBindImageMemory");
            XRAY_TEXTURE_PROC(map_memory, "vkMapMemory");
            XRAY_TEXTURE_PROC(unmap_memory, "vkUnmapMemory");
            XRAY_TEXTURE_PROC(create_image_view, "vkCreateImageView");
            XRAY_TEXTURE_PROC(destroy_image_view, "vkDestroyImageView");
            XRAY_TEXTURE_PROC(allocate_command_buffers, "vkAllocateCommandBuffers");
            XRAY_TEXTURE_PROC(free_command_buffers, "vkFreeCommandBuffers");
            XRAY_TEXTURE_PROC(begin_command_buffer, "vkBeginCommandBuffer");
            XRAY_TEXTURE_PROC(end_command_buffer, "vkEndCommandBuffer");
            XRAY_TEXTURE_PROC(cmd_pipeline_barrier, "vkCmdPipelineBarrier");
            XRAY_TEXTURE_PROC(cmd_copy_buffer_to_image, "vkCmdCopyBufferToImage");
            XRAY_TEXTURE_PROC(queue_submit, "vkQueueSubmit");
            XRAY_TEXTURE_PROC(create_fence, "vkCreateFence");
            XRAY_TEXTURE_PROC(destroy_fence, "vkDestroyFence");
            XRAY_TEXTURE_PROC(get_fence_status, "vkGetFenceStatus");
            XRAY_TEXTURE_PROC(wait_for_fences, "vkWaitForFences");
#undef XRAY_TEXTURE_PROC
            if (!texture_dispatch.create_buffer || !texture_dispatch.destroy_buffer ||
                !texture_dispatch.get_buffer_memory_requirements || !texture_dispatch.create_image ||
                !texture_dispatch.destroy_image || !texture_dispatch.get_image_memory_requirements ||
                !texture_dispatch.allocate_memory || !texture_dispatch.free_memory ||
                !texture_dispatch.bind_buffer_memory || !texture_dispatch.bind_image_memory ||
                !texture_dispatch.map_memory || !texture_dispatch.unmap_memory ||
                !texture_dispatch.create_image_view || !texture_dispatch.destroy_image_view ||
                !texture_dispatch.allocate_command_buffers || !texture_dispatch.free_command_buffers ||
                !texture_dispatch.begin_command_buffer || !texture_dispatch.end_command_buffer ||
                !texture_dispatch.cmd_pipeline_barrier || !texture_dispatch.cmd_copy_buffer_to_image ||
                !texture_dispatch.queue_submit || !texture_dispatch.create_fence ||
                !texture_dispatch.destroy_fence || !texture_dispatch.get_fence_status ||
                !texture_dispatch.wait_for_fences)
                return fail("required Vulkan texture upload procedures are unavailable");

    // A normal game boot has a mounted VFS. The no-game smoke boot does not.
    // Exercise an actual engine-owned DDS through the same archive-aware reader
    // used by the other renderers when the standard fallback texture exists.
    if (FS.path_exist("$game_textures$"))
    {
        string_path texture_path;
        if (FS.exist(texture_path, "$game_textures$", "ed\\ed_not_existing_texture", ".dds"))
        {
            IReader* reader = FS.r_open(texture_path);
            if (!reader)
                return fail("engine VFS could not open the fallback DDS");
            std::string texture_error;
            const bool uploaded = xray::render::vulkan::upload_engine_texture(device, queue,
                frame_context.command_pool(), physical_selection.memory, texture_dispatch,
                reader->pointer(), reader->length(), physical_features.textureCompressionBC,
                engine_texture, texture_uploads, image_states, texture_error);
            FS.r_close(reader);
            if (!uploaded)
                return fail("engine DDS Vulkan upload failed: " + texture_error);
            Msg("[renderer-vulkan] engine VFS DDS uploaded to sampled image: %s", texture_path);
        }
    }

    FrameCallbackAudit callback_audit;
    PixelReadback pixel_readback;
    if (no_game_vfs)
    {
        const xray::render::vulkan::ShaderModuleDispatch shaders{
            load_device_proc<PFN_vkCreateShaderModule>(device, get_device_proc, "vkCreateShaderModule"),
            load_device_proc<PFN_vkDestroyShaderModule>(device, get_device_proc, "vkDestroyShaderModule")};
        xray::render::vulkan::SmokeTriangleDispatch draw;
        std::string draw_error;
        if (!shaders.create || !shaders.destroy ||
            !xray::render::vulkan::load_smoke_triangle_dispatch(device, get_device_proc, draw, draw_error))
            return fail(draw_error.empty() ? "Vulkan smoke shader procedures are unavailable" : draw_error);
        if (!smoke_vertex.initialize(device, shaders, xray::render::vulkan::smoke::TriangleVertex,
                sizeof(xray::render::vulkan::smoke::TriangleVertex), draw_error) ||
            !smoke_fragment.initialize(device, shaders, xray::render::vulkan::smoke::TriangleFragment,
                sizeof(xray::render::vulkan::smoke::TriangleFragment), draw_error) ||
            !smoke_triangle.initialize(device, frame_context.render_pass(), smoke_vertex.handle(),
                smoke_fragment.handle(), draw, draw_error))
            return fail(draw_error);
        callback_audit.triangle = &smoke_triangle;
        xray::render::vulkan::BufferResourceDispatch buffer_dispatch{
            load_device_proc<PFN_vkCreateBuffer>(device, get_device_proc, "vkCreateBuffer"),
            load_device_proc<PFN_vkDestroyBuffer>(device, get_device_proc, "vkDestroyBuffer"),
            load_device_proc<PFN_vkGetBufferMemoryRequirements>(device, get_device_proc,
                "vkGetBufferMemoryRequirements"),
            load_device_proc<PFN_vkAllocateMemory>(device, get_device_proc, "vkAllocateMemory"),
            load_device_proc<PFN_vkFreeMemory>(device, get_device_proc, "vkFreeMemory"),
            load_device_proc<PFN_vkBindBufferMemory>(device, get_device_proc, "vkBindBufferMemory"),
            load_device_proc<PFN_vkMapMemory>(device, get_device_proc, "vkMapMemory"),
            load_device_proc<PFN_vkUnmapMemory>(device, get_device_proc, "vkUnmapMemory")};
        if (!pixel_buffer.initialize(device, 8, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, physical_selection.memory, buffer_dispatch, draw_error))
            return fail(draw_error);
        xray::render::vulkan::DdsTexture ui_source;
        ui_source.format = VK_FORMAT_R8G8B8A8_UNORM;
        ui_source.extent = {2, 2, 1};
        ui_source.mip_levels = 1;
        ui_source.pixels = {255, 230, 255, 255, 255, 230, 255, 255,
                            255, 230, 255, 255, 255, 230, 255, 255};
        VkBufferImageCopy ui_copy{};
        ui_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ui_copy.imageSubresource.layerCount = 1;
        ui_copy.imageExtent = ui_source.extent;
        ui_source.copies.push_back(ui_copy);
        if (!xray::render::vulkan::upload_texture(device, queue, frame_context.command_pool(),
                physical_selection.memory, texture_dispatch, ui_source, ui_texture,
                texture_uploads, image_states, draw_error))
            return fail(draw_error);
        const auto create_sampler = load_device_proc<PFN_vkCreateSampler>(device, get_device_proc,
            "vkCreateSampler");
        destroy_sampler = load_device_proc<PFN_vkDestroySampler>(device, get_device_proc,
            "vkDestroySampler");
        if (!create_sampler || !destroy_sampler)
            return fail("Vulkan UI sampler procedures are unavailable");
        VkSamplerCreateInfo ui_sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        ui_sampler_info.magFilter = VK_FILTER_NEAREST;
        ui_sampler_info.minFilter = VK_FILTER_NEAREST;
        ui_sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        ui_sampler_info.addressModeU = ui_sampler_info.addressModeV =
            ui_sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        if (create_sampler(device, &ui_sampler_info, nullptr, &copy_sampler) != VK_SUCCESS)
            return fail("could not create Vulkan UI sampler");
        xray::render::vulkan::ScenePassDispatch scene_dispatch;
        if (!xray::render::vulkan::load_scene_pass_dispatch(device, get_device_proc,
                scene_dispatch, draw_error) ||
            !scene_vertex.initialize(device, shaders, xray::render::vulkan::scene_shaders::SceneVertex,
                sizeof(xray::render::vulkan::scene_shaders::SceneVertex), draw_error) ||
            !scene_fragment.initialize(device, shaders, xray::render::vulkan::scene_shaders::SceneFragment,
                sizeof(xray::render::vulkan::scene_shaders::SceneFragment), draw_error) ||
            !ui_vertex.initialize(device, shaders, xray::render::vulkan::scene_shaders::UiVertex,
                sizeof(xray::render::vulkan::scene_shaders::UiVertex), draw_error) ||
            !ui_fragment.initialize(device, shaders, xray::render::vulkan::scene_shaders::UiFragment,
                sizeof(xray::render::vulkan::scene_shaders::UiFragment), draw_error) ||
            !scene_pass.initialize(device, frame_context.render_pass(), scene_vertex.handle(),
                scene_fragment.handle(), ui_vertex.handle(), ui_fragment.handle(), scene_dispatch, draw_error,
                frame_context.depth_format() != VK_FORMAT_UNDEFINED))
            return fail(draw_error);
        if (!scene_pass.create_ui_texture_set(ui_texture.view, copy_sampler,
                callback_audit.ui_texture_set, draw_error))
            return fail(draw_error);

        using xray::render::vulkan::SceneVertex;
        using xray::render::vulkan::UiVertex;
        const SceneVertex geometry[] = {
            {{-0.65f, -0.65f, 0}, {0, 0, 1}, {0.6f, 0.7f, 1.0f, 1}},
            {{0.65f, -0.65f, 0}, {0, 0, 1}, {0.6f, 0.7f, 1.0f, 1}},
            {{0, 0.7f, 0}, {0, 0, 1}, {0.6f, 0.7f, 1.0f, 1}}
        };
        const uint16_t geometry_indices[]{0, 1, 2};
        const UiVertex overlay[] = {
            {{20, 20}, {0, 0}, 0xcce0a040u},
            {{200, 20}, {1, 0}, 0xcce0a040u},
            {{200, 80}, {1, 1}, 0xcce0a040u},
            {{20, 80}, {0, 1}, 0xcce0a040u}
        };
        const uint16_t overlay_indices[]{0, 1, 2, 0, 2, 3};
        const auto make_buffer = [&](xray::render::vulkan::BufferResource& buffer,
            const void* bytes, size_t count, VkBufferUsageFlags usage)
        {
            return buffer.initialize(device, count, usage,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                physical_selection.memory, buffer_dispatch, draw_error) &&
                buffer.write(0, bytes, count, draw_error);
        };
        if (!make_buffer(scene_vertices, geometry, sizeof(geometry), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
            !make_buffer(scene_indices, geometry_indices, sizeof(geometry_indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT) ||
            !make_buffer(ui_vertices, overlay, sizeof(overlay), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
            !make_buffer(ui_indices, overlay_indices, sizeof(overlay_indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
            return fail(draw_error);
        auto& constants = callback_audit.scene_constants;
        constants.model_view_projection[0] = constants.model_view_projection[5] =
            constants.model_view_projection[10] = constants.model_view_projection[15] = 1.0f;
        constants.light_direction_ambient[2] = -1.0f;
        constants.light_direction_ambient[3] = 0.2f;
        constants.light_color[0] = constants.light_color[1] = constants.light_color[2] = 0.8f;
        callback_audit.scene = &scene_pass;
        callback_audit.scene_vertices = scene_vertices.handle();
        callback_audit.scene_indices = scene_indices.handle();
        callback_audit.ui_vertices = ui_vertices.handle();
        callback_audit.ui_indices = ui_indices.handle();
        pixel_readback = {pixel_buffer.handle(), frame_dispatch.cmd_copy_image_to_buffer,
            frame_dispatch.cmd_pipeline_barrier, 0};
        Msg("[renderer-vulkan] indexed geometry, directional lighting and UI pipelines initialized");
    }
    if (engine_texture.view && FS.path_exist("$game_shaders$"))
    {
        string_path vertex_path, fragment_path;
        const bool has_vertex = FS.exist(vertex_path, "$game_shaders$", "r3\\screen_copy_vk.vs", ".spv");
        const bool has_fragment = FS.exist(fragment_path, "$game_shaders$", "r3\\screen_copy_vk.ps", ".spv");
        if (has_vertex != has_fragment)
            return fail("screen-copy SPIR-V files must be installed as a pair");
        if (has_vertex)
        {
            const xray::render::vulkan::ShaderModuleDispatch shader_dispatch{
                load_device_proc<PFN_vkCreateShaderModule>(device, get_device_proc, "vkCreateShaderModule"),
                load_device_proc<PFN_vkDestroyShaderModule>(device, get_device_proc, "vkDestroyShaderModule")};
            const auto create_sampler = load_device_proc<PFN_vkCreateSampler>(device, get_device_proc,
                "vkCreateSampler");
            destroy_sampler = load_device_proc<PFN_vkDestroySampler>(device, get_device_proc,
                "vkDestroySampler");
            xray::render::vulkan::ScreenCopyDispatch copy_dispatch;
            std::string copy_error;
            if (!shader_dispatch.create || !shader_dispatch.destroy || !create_sampler || !destroy_sampler ||
                !xray::render::vulkan::load_screen_copy_dispatch(device, get_device_proc,
                    copy_dispatch, copy_error))
                return fail(copy_error.empty() ? "Vulkan shader or sampler procedures are unavailable" : copy_error);
            if (!load_shader_from_vfs("r3\\screen_copy_vk.vs", device, shader_dispatch,
                    copy_vertex, copy_error) ||
                !load_shader_from_vfs("r3\\screen_copy_vk.ps", device, shader_dispatch,
                    copy_fragment, copy_error))
                return fail(copy_error);
            VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            sampler_info.magFilter = VK_FILTER_NEAREST;
            sampler_info.minFilter = VK_FILTER_NEAREST;
            sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sampler_info.maxLod = 0.0f;
            if (create_sampler(device, &sampler_info, nullptr, &copy_sampler) != VK_SUCCESS)
                return fail("Vulkan screen-copy sampler creation failed");
            if (!screen_copy.initialize(device, frame_context.render_pass(), engine_texture.view,
                    copy_sampler, copy_vertex.handle(), copy_fragment.handle(), copy_dispatch, copy_error))
                return fail(copy_error);
            callback_audit.screen_copy = &screen_copy;
            Msg("[renderer-vulkan] screen-copy pipeline bound to engine VFS DDS");
        }
        else
            Msg("[renderer-vulkan] compiled screen-copy shaders absent; clear/present probe continues");
    }

    VkClearColorValue clear{};
    clear.float32[0] = 0.08f;
    clear.float32[1] = 0.18f;
    clear.float32[2] = 0.32f;
    clear.float32[3] = 1.0f;
    for (uint32_t frame = 0; frame <= xray::render::vulkan::FrameContext::FramesInFlight; ++frame)
    {
        xray::render::vulkan::FrameStatus frame_status{};
        if (!frame_context.render_frame(clear, frame_status, frame_error,
                audit_frame_callback, &callback_audit,
                no_game_vfs && frame == 0 ? record_pixel_readback : nullptr,
                no_game_vfs && frame == 0 ? &pixel_readback : nullptr))
            return fail(frame_error);
        if (frame_status != xray::render::vulkan::FrameStatus::Presented)
            return fail("Vulkan surface changed during the smoke test");
        if (!callback_audit.valid || callback_audit.calls != frame + 1)
            return fail("Vulkan frame recorder received an invalid frame context");
    }
    if (frame_dispatch.device_wait_idle(device) != VK_SUCCESS)
        return fail("Vulkan queue did not become idle after present");

    if (no_game_vfs)
    {
        std::array<uint8_t, 8> pixel{};
        if (pixel_readback.copies != 1 ||
            !pixel_buffer.read(0, pixel.data(), pixel.size(), frame_error))
            return fail("Vulkan triangle center pixel readback failed");
        const uint8_t red = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[2] : pixel[0];
        const uint8_t green = pixel[1];
        const uint8_t blue = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[0] : pixel[2];
        Msg("[renderer-vulkan] center pixel RGBA=(%u,%u,%u,%u)", red, green, blue, pixel[3]);
        if (red < 50 || green < 60 || blue < 90 || pixel[3] < 250)
            return fail("Vulkan lit geometry center pixel did not contain the drawn color");
        const uint8_t ui_red = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[6] : pixel[4];
        const uint8_t ui_green = pixel[5];
        const uint8_t ui_blue = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[4] : pixel[6];
        Msg("[renderer-vulkan] UI pixel RGBA=(%u,%u,%u,%u)",
            ui_red, ui_green, ui_blue, pixel[7]);
        if (ui_green < 100 || ui_blue < 150 || pixel[7] < 200)
            return fail("Vulkan UI pixel did not contain the alpha-blended overlay");

        // Exercise the real two-attachment geometry pass, shader-read
        // transition and fullscreen light pass on the same present submission.
        std::string deferred_error;
        const auto create_sampler = load_device_proc<PFN_vkCreateSampler>(device, get_device_proc,
            "vkCreateSampler");
        xray::render::vulkan::ScenePassDispatch deferred_dispatch;
        if (!xray::render::vulkan::load_scene_pass_dispatch(device, get_device_proc,
                deferred_dispatch, deferred_error) ||
            !deferred_targets.initialize(physical_device, device, frame_context.extent(),
                static_cast<uint32_t>(frame_context.image_count()), frame_context.depth_format(),
                physical_selection.memory, frame_dispatch, create_sampler,
                destroy_sampler, deferred_error))
            return fail(deferred_error);
        const xray::render::vulkan::ShaderModuleDispatch shader_dispatch{
            load_device_proc<PFN_vkCreateShaderModule>(device, get_device_proc, "vkCreateShaderModule"),
            load_device_proc<PFN_vkDestroyShaderModule>(device, get_device_proc, "vkDestroyShaderModule")};
        xray::render::vulkan::DeferredShaderFactory factory;
        xray::render::vulkan::GameShaderResources resources;
        xray::render::vulkan::configure_engine_shader_resources(device, shader_dispatch, resources);
        if (!factory.create(device, shader_dispatch, deferred_dispatch, deferred_targets.render_pass(), frame_context.render_pass(), deferred_pass, resources,
                            deferred_error) ||
            !deferred_targets.bind_lighting(deferred_pass, deferred_error))
            return fail(deferred_error);
        VkDescriptorSet material = VK_NULL_HANDLE;
        if (!deferred_pass.material(ui_texture.view, copy_sampler, material, deferred_error))
            return fail(deferred_error);
        const xray::render::vulkan::LevelVertex vertices[]{
            {{-0.65f, -0.65f, 0}, {0, 0, 1}, {0, 0}},
            {{0.65f, -0.65f, 0}, {0, 0, 1}, {1, 0}},
            {{0, 0.7f, 0}, {0, 0, 1}, {0.5f, 1}}
        };
        const uint32_t indices[]{0, 1, 2};
        const xray::render::vulkan::BufferResourceDispatch buffer_dispatch{
            load_device_proc<PFN_vkCreateBuffer>(device, get_device_proc, "vkCreateBuffer"),
            load_device_proc<PFN_vkDestroyBuffer>(device, get_device_proc, "vkDestroyBuffer"),
            load_device_proc<PFN_vkGetBufferMemoryRequirements>(device, get_device_proc,
                "vkGetBufferMemoryRequirements"),
            load_device_proc<PFN_vkAllocateMemory>(device, get_device_proc, "vkAllocateMemory"),
            load_device_proc<PFN_vkFreeMemory>(device, get_device_proc, "vkFreeMemory"),
            load_device_proc<PFN_vkBindBufferMemory>(device, get_device_proc, "vkBindBufferMemory"),
            load_device_proc<PFN_vkMapMemory>(device, get_device_proc, "vkMapMemory"),
            load_device_proc<PFN_vkUnmapMemory>(device, get_device_proc, "vkUnmapMemory")};
        if (!deferred_vertices.initialize(device, sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                physical_selection.memory, buffer_dispatch, deferred_error) ||
            !deferred_vertices.write(0, vertices, sizeof(vertices), deferred_error) ||
            !deferred_indices.initialize(device, sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                physical_selection.memory, buffer_dispatch, deferred_error) ||
            !deferred_indices.write(0, indices, sizeof(indices), deferred_error))
            return fail(deferred_error);
        DeferredAudit deferred{};
        deferred.targets = &deferred_targets;
        deferred.pass = &deferred_pass;
        deferred.ui = &scene_pass;
        deferred.vertices = deferred_vertices.handle();
        deferred.indices = deferred_indices.handle();
        deferred.ui_vertices = ui_vertices.handle();
        deferred.ui_indices = ui_indices.handle();
        deferred.material = material;
        deferred.ui_texture = callback_audit.ui_texture_set;
        deferred.mvp[0] = deferred.mvp[5] = deferred.mvp[10] = deferred.mvp[15] = 1;
        deferred.light.direction_ambient[2] = -1;
        deferred.light.direction_ambient[3] = 0.2f;
        deferred.light.color[0] = deferred.light.color[1] = deferred.light.color[2] = 0.8f;
        xray::render::vulkan::FrameStatus deferred_status{};
        if (!frame_context.render_frame(clear, deferred_status, deferred_error,
                record_deferred_lighting, &deferred, record_pixel_readback, &pixel_readback,
                record_deferred_geometry, &deferred) ||
            deferred_status != xray::render::vulkan::FrameStatus::Presented ||
            !deferred.valid || deferred.geometry_calls != 1 || deferred.lighting_calls != 1 ||
            frame_dispatch.device_wait_idle(device) != VK_SUCCESS ||
            !pixel_buffer.read(0, pixel.data(), pixel.size(), deferred_error))
            return fail(deferred_error.empty() ? "Vulkan deferred smoke recording failed" : deferred_error);
        const uint8_t deferred_red = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[2] : pixel[0];
        const uint8_t deferred_green = pixel[1];
        const uint8_t deferred_blue = frame_context.format() == VK_FORMAT_B8G8R8A8_UNORM ? pixel[0] : pixel[2];
        if (deferred_red < 190 || deferred_green < 170 || deferred_blue < 190)
            return fail("Vulkan deferred light pixel does not contain textured geometry");
        Msg("[renderer-vulkan] deferred G-buffer and lighting pixel RGB=(%u,%u,%u)",
            deferred_red, deferred_green, deferred_blue);
    }

    Msg("[renderer-vulkan] %s PASS: %s, Vulkan %u.%u.%u",
        no_game_vfs ? "forward and deferred geometry, UI, pixel readbacks and present" : "frame submit and present",
        physical_properties.deviceName,
        VK_VERSION_MAJOR(physical_properties.apiVersion), VK_VERSION_MINOR(physical_properties.apiVersion),
        VK_VERSION_PATCH(physical_properties.apiVersion));
    reason = no_game_vfs ? "Vulkan geometry, deferred lighting and textured UI readbacks passed" :
        "three Vulkan frame-context submits and presents passed";
    cleanup();
    return true;
#endif
}
}

#endif
