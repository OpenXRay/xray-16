#include "src/Layers/xrRenderVK/VulkanWindowDevice.h"
#include <SDL.h>
#include <fstream>
#include <string>

int main()
{
    SDL_setenv("XRAY_VK_VALIDATION", "1", 1);
    if (SDL_Init(SDL_INIT_VIDEO)) { SDL_Log("SDL_Init: %s", SDL_GetError()); return 1; }
    SDL_Window* window = SDL_CreateWindow("OpenXRay Vulkan validation host",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, SDL_WINDOW_VULKAN);
    if (!window) { SDL_Log("SDL_CreateWindow: %s", SDL_GetError()); return 2; }
    char* directory = SDL_GetPrefPath("OpenXRay", "validation");
    if (!directory) return 5;
    const std::string path = std::string(directory) + "vulkan-vuid.log";
    SDL_free(directory);
    std::ofstream(path, std::ios::trunc).close();
    xray::render::vulkan::VulkanWindowDevice device;
    std::string error;
    if (!device.initialize(window, {320, 240}, false, error))
    {
        SDL_Log("Vulkan validation host initialization: %s", error.c_str());
        return 3;
    }
    const VkClearColorValue clear{};
    for (unsigned cycle = 0; cycle != 3; ++cycle)
    {
        xray::render::vulkan::FrameStatus status{};
        if (!device.frame().render_frame(clear, status, error) ||
            !device.recreate_surface({320 + cycle * 8, 240 + cycle * 8}, error) ||
            !device.frame().image_count())
        {
            SDL_Log("Vulkan surface replacement cycle %u: %s", cycle, error.c_str());
            return 7;
        }
    }
    {
        std::ifstream before_fault(path);
        std::string line;
        while (std::getline(before_fault, line))
            if (line.find("VUID-") != std::string::npos) return 8;
    }
    const auto create = reinterpret_cast<PFN_vkCreateSemaphore>(
        device.device_proc()(device.device(), "vkCreateSemaphore"));
    const auto destroy = reinterpret_cast<PFN_vkDestroySemaphore>(
        device.device_proc()(device.device(), "vkDestroySemaphore"));
    if (!create || !destroy) return 4;
    // Validation must report this wrong sType and write the VUID to disk.
    VkSemaphoreCreateInfo invalid{};
    invalid.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    const auto result = create(device.device(), &invalid, nullptr, &semaphore);
    if (result == VK_SUCCESS && semaphore) destroy(device.device(), semaphore, nullptr);
    device.destroy();
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::ifstream log(path);
    std::string line;
    while (std::getline(log, line))
        if (line.find("VUID-") != std::string::npos) return 0;
    return 6;
}
