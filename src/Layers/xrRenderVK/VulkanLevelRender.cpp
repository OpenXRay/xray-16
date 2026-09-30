#include "xrEngine/stdafx.h"
#include "VulkanLevelRender.h"
#include "VulkanGameDevice.h"
#include "VulkanVisual.h"
#include "xrEngine/device.h"

#include <SDL.h>

#include <cstring>

namespace xray::render::vulkan
{
VulkanLevelRender::~VulkanLevelRender()
{
    Destroy();
}

void VulkanLevelRender::Create(SDL_Window* window, u32& width, u32& height,
    float& half_width, float& half_height)
{
    if (!window)
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer requires the engine SDL window");
        return;
    }
    if (!(SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan renderer requires SDL_WINDOW_VULKAN on the engine window");
        return;
    }

    Destroy();
    auto device = std::make_unique<VulkanGameDevice>();
    std::string error;
    if (!device->initialize(window, {width, height}, error))
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan device creation failed: %s", error.c_str());
        return;
    }

    const VkExtent2D extent = device->window().frame().extent();
    if (!extent.width || !extent.height)
    {
        xrDebug::Fatal(DEBUG_INFO, "Vulkan swapchain has an empty extent");
        return;
    }
    width = extent.width;
    height = extent.height;
    half_width = static_cast<float>(width) * 0.5f;
    half_height = static_cast<float>(height) * 0.5f;

    owned_game_device_ = std::move(device);
    bind_level_device(*owned_game_device_);
}

void VulkanLevelRender::Destroy()
{
    if (device_resources_ready_)
        OnDeviceDestroy(false);
    else
        level_Unload();
    if (game_device_)
        game_device_->use_scene_visibility(false);
    game_device_ = nullptr;
    device_ = VK_NULL_HANDLE;
    queue_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE;
    memory_ = {};
    upload_ = {};
    textures_ = nullptr;
    pass_ = nullptr;
    wait_idle_ = nullptr;
    if (owned_game_device_)
    {
        owned_game_device_->destroy();
        owned_game_device_.reset();
    }
}

void VulkanLevelRender::SetupStates()
{
    R_ASSERT2(game_device_ && game_device_->window().device(),
        "Vulkan renderer state setup requires a created Vulkan device");
    game_device_->ui().setup_states();
}

void VulkanLevelRender::OnDeviceCreate(pcstr)
{
    R_ASSERT2(game_device_ && game_device_->window().device(),
        "Vulkan renderer resources require a created Vulkan device");
    if (device_resources_ready_)
        return;

    game_device_->ui().CreateUIGeom();
    device_resources_ready_ = true;
}

void VulkanLevelRender::OnDeviceDestroy(bool)
{
    if (!game_device_)
    {
        level_Unload();
        device_resources_ready_ = false;
        return;
    }

    // level_Unload waits for submitted work before dropping level buffers.
    level_Unload();
    game_device_->ui().DestroyUIGeom();
    device_resources_ready_ = false;
}

void VulkanLevelRender::bind_level_device(VulkanGameDevice& resources)
{
    auto& window = resources.window();
    bind_level_device(window.device(), window.queue(), window.frame().command_pool(),
        window.physical().memory, resources.buffer_upload(), resources.textures(),
        resources.deferred(), resources.wait_idle());
    game_device_ = &resources;
    resources.use_scene_visibility(true);
}

void VulkanLevelRender::bind_level_device(VkDevice device, VkQueue queue, VkCommandPool pool,
    const VkPhysicalDeviceMemoryProperties& memory, const BufferUploadDispatch& upload,
    GameTextureFactory& textures, DeferredPass& pass, PFN_vkDeviceWaitIdle wait_idle)
{
    level_Unload();
    if (game_device_)
        game_device_->use_scene_visibility(false);
    game_device_ = nullptr;
    device_ = device;
    queue_ = queue;
    pool_ = pool;
    memory_ = memory;
    upload_ = upload;
    textures_ = &textures;
    pass_ = &pass;
    wait_idle_ = wait_idle;
}

void VulkanLevelRender::level_Load(IReader* reader)
{
    R_ASSERT2(reader && device_ && queue_ && pool_ && textures_ && pass_ && wait_idle_,
        "Vulkan level load requires an initialized gameplay device and resources");
    // Replacing a level must wait for every submitted draw before its buffers
    // and visual identities are released. Uploads use the same graphics queue.
    R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan queue did not become idle before level load");
    std::string error;
    if (!level_.load(*reader, device_, queue_, pool_, memory_, upload_, *textures_, *pass_, error))
        xrDebug::Fatal(DEBUG_INFO, "Vulkan level load failed: %s", error.c_str());
}

void VulkanLevelRender::level_Unload()
{
    if (device_ && wait_idle_)
        R_ASSERT2(wait_idle_(device_) == VK_SUCCESS, "Vulkan queue did not become idle before level unload");
    level_.destroy();
}

IRenderVisual* VulkanLevelRender::getVisual(int index)
{
    return index >= 0 ? level_.get_visual(static_cast<size_t>(index)) : nullptr;
}

void VulkanLevelRender::add_Visual(u32, IRenderable*, IRenderVisual* visual, Fmatrix& world)
{
    R_ASSERT2(game_device_, "Vulkan level visuals require a bound gameplay device");
    const auto* level_visual = dynamic_cast<const VulkanVisual*>(visual);
    R_ASSERT2(level_visual && &level_visual->owner() == &level_,
        "Vulkan scene submission received a visual outside this level");
    Fmatrix mvp;
    mvp.mul(Device.mFullTransform, world);
    static_assert(sizeof(Fmatrix) == 16 * sizeof(float));
    float transform[16];
    std::memcpy(transform, &mvp, sizeof(transform));
    game_device_->queue_level_visual(level_visual->index(), transform);
}
}
