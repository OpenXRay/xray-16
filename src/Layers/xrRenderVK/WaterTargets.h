#pragma once

#include "FrameContext.h"
#include "BufferResource.h"

namespace xray::render::vulkan
{
struct alignas(16) WaterSceneUniform
{
    float view_projection[16]{};
    float inverse_view_projection[16]{};
    float camera_position[4]{};
};
static_assert(sizeof(WaterSceneUniform) == 144);
// Captures the lit opaque frame into two independently owned sampled images
// between the lighting and transparent passes. Reflection is a screen-space
// estimate; both targets are refreshed for each acquired swapchain image.
class WaterTargets
{
public:
    ~WaterTargets() { destroy(); }
    bool initialize(VkPhysicalDevice physical, VkDevice device, VkFormat format, VkExtent2D extent, uint32_t count,
        const VkPhysicalDeviceMemoryProperties& memory, const FrameDispatch& dispatch,
        const BufferResourceDispatch& buffers, std::string& error);
    bool write_uniform(uint32_t index, const WaterSceneUniform& value, std::string& error);
    bool capture(VkCommandBuffer command, VkImage source, uint32_t index) const;
    VkImageView refraction(uint32_t index) const;
    VkImageView reflection(uint32_t index) const;
    VkBuffer uniform(uint32_t index) const;
    void destroy();

private:
    struct Image
    {
        VkImage image{};
        VkDeviceMemory memory{};
        VkImageView view{};
    };
    struct Target { Image refraction, reflection; BufferResource constants; };
    bool make_image(Image& image, std::string& error);
    VkDevice device_{};
    VkFormat format_{VK_FORMAT_UNDEFINED};
    VkExtent2D extent_{};
    VkPhysicalDeviceMemoryProperties memory_{};
    FrameDispatch vk_{};
    std::vector<Target> targets_;
};
}
