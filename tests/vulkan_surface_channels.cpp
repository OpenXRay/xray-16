// Offscreen execution of the shipped fragment SPIR-V with 20 distinguishable
// sampled slots. Run with a software Vulkan ICD when no physical GPU exists.
#include <vulkan/vulkan.h>
#include "../src/Layers/xrRenderVK/DistortionShaders.h"
#include "../src/Layers/xrRenderVK/ParticleSprite.h"
#include "../src/Layers/xrRenderVK/LevelLights.h"
#include "../src/Layers/xrRenderVK/ModelPoseConstants.h"
#include "../src/Layers/xrRenderVK/GeometryDepth.h"
#include "../src/Layers/xrRenderVK/SceneShaders.h"
#include "../src/Layers/xrRenderVK/WeatherShaders.h"
#include "../src/Layers/xrRenderVK/WaterShaders.h"
#include "../src/Layers/xrRenderVK/RenderPolicy.h"
#include "../src/Layers/xrRenderVK/DeferredShaders.h"
#include "../src/Layers/xrRenderVK/LocalLightShaders.h"
#include "../src/Layers/xrRenderVK/LocalShadowTargets.h"
#include "../src/Layers/xrRenderVK/DetailInstance.h"
#include "../src/Layers/xrRenderVK/ForwardLighting.h"
#include "vulkan_motion_scene.h"
#include "../src/Include/xrRender/WaterContactEffect.h"
#include <array>
#include <cstdlib>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static void check(VkResult r)
{
    if (r != VK_SUCCESS)
        throw std::runtime_error("Vulkan result " + std::to_string(r));
}
static std::vector<uint32_t> shader(const std::string &name)
{
    std::ifstream input(name, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 20 || input.tellg() % 4)
        throw std::runtime_error("SPIR-V: " + name);
    std::vector<uint32_t> code(size_t(input.tellg()) / 4);
    input.seekg(0);
    input.read(reinterpret_cast<char *>(code.data()), std::streamsize(code.size() * 4));
    return code;
}
struct Memory
{
    VkDevice device{};
    VkPhysicalDeviceMemoryProperties types{};
    uint32_t select(uint32_t bits, VkMemoryPropertyFlags required)
    {
        for (uint32_t i = 0; i < types.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (types.memoryTypes[i].propertyFlags & required) == required)
                return i;
        throw std::runtime_error("Vulkan memory type unavailable");
    }
    VkDeviceMemory allocate(VkMemoryRequirements requirements, VkMemoryPropertyFlags flags)
    {
        VkMemoryAllocateInfo info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = select(requirements.memoryTypeBits, flags);
        VkDeviceMemory result{};
        check(vkAllocateMemory(device, &info, nullptr, &result));
        return result;
    }
};
struct Image
{
    VkImage image{};
    VkImageView view{};
    VkDeviceMemory memory{};
    uint32_t width{1}, height{1};
};
static Image image(Memory &memory, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags flags, uint32_t layers = 1, uint32_t width = 1, uint32_t levels = 1,
                   uint32_t height = 1)
{
    Image result;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {width, layers == 6 ? width : height, 1};
    result.width = width;
    result.height = info.extent.height;
    info.mipLevels = levels;
    info.arrayLayers = layers;
    if (layers == 6)
        info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = tiling;
    info.usage = usage;
    info.initialLayout = tiling == VK_IMAGE_TILING_LINEAR ? VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
    check(vkCreateImage(memory.device, &info, nullptr, &result.image));
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(memory.device, result.image, &requirements);
    result.memory = memory.allocate(requirements, flags);
    check(vkBindImageMemory(memory.device, result.image, result.memory, 0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = result.image;
    view.viewType = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange.aspectMask =
        format == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = levels;
    view.subresourceRange.layerCount = layers;
    check(vkCreateImageView(memory.device, &view, nullptr, &result.view));
    return result;
}
static void fill(Memory &memory, const Image &image, const std::array<float, 4> &rgba, uint32_t layers = 1,
                 uint32_t width = 1)
{
    void *mapped{};
    check(vkMapMemory(memory.device, image.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
    for (uint32_t face = 0; face < layers; ++face)
    {
        VkImageSubresource subresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, face};
        VkSubresourceLayout layout{};
        vkGetImageSubresourceLayout(memory.device, image.image, &subresource, &layout);
        for (uint32_t y = 0; y < image.height; ++y)
            for (uint32_t x = 0; x < image.width; ++x)
                std::memcpy(static_cast<uint8_t *>(mapped) + layout.offset + y * layout.rowPitch +
                                x * sizeof(float) * 4,
                            rgba.data(), sizeof(float) * 4);
    }
    vkUnmapMemory(memory.device, image.memory);
}
struct Vertex
{
    float position[3], normal[3], uv[2], lightmap_uv[2], baked[4];
    uint16_t bones[4]{};
    float weights[4]{1, 0, 0, 0};
    float tangent[3]{0, 1, 0}, bitangent[3]{-1, 0, 0};
    int8_t static_tangent[4]{}, static_bitangent[4]{};
    float clip_depth_w[2]{0, 1};
};
struct Push
{
    float mvp[16];
    float threshold{}, scale{};
    float padding[2]{};
    float modes[4]{}, factors[4]{}, extra[4]{}, matrices[28]{}, rotation[4]{0, 0, 0, 1};
};
static_assert(sizeof(Push) == 256);
static void near(float actual, float expected, const char *label)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > .015f)
        throw std::runtime_error(std::string(label) + " expected " + std::to_string(expected) + ", got " +
                                 std::to_string(actual));
}
int main(int argc, char **argv)
{
    try
    {
        if (argc < 2 || argc > 4)
            throw std::runtime_error(
                "usage: vulkan_surface_channels <vk shader directory> [menu-preview.ppm] [weather-preview.ppm]");
        VkInstance instance{};
        VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        check(vkCreateInstance(&instance_info, nullptr, &instance));
        uint32_t count{};
        check(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        if (!count)
            throw std::runtime_error("no Vulkan physical device");
        std::vector<VkPhysicalDevice> devices(count);
        check(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
        VkPhysicalDevice physical = devices[0];
        uint32_t families{};
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &families, nullptr);
        std::vector<VkQueueFamilyProperties> properties(families);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &families, properties.data());
        uint32_t family = 0;
        while (family < families && !(properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            ++family;
        if (family == families)
            throw std::runtime_error("no graphics queue");
        float priority = 1;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        VkPhysicalDeviceFeatures enabled{};
        vkGetPhysicalDeviceFeatures(physical, &enabled);
        if (!enabled.independentBlend)
            throw std::runtime_error("fixture requires independent blend");
        device_info.pEnabledFeatures = &enabled;
        VkDevice device{};
        check(vkCreateDevice(physical, &device_info, nullptr, &device));
        VkQueue queue{};
        vkGetDeviceQueue(device, family, 0, &queue);
        Memory memory{device};
        vkGetPhysicalDeviceMemoryProperties(physical, &memory.types);
        constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkFormatProperties support{};
        vkGetPhysicalDeviceFormatProperties(physical, format, &support);
        if (!(support.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
            !(support.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
            throw std::runtime_error("float sampled/attachment images unsupported");
        std::array<Image, 20> input;
        for (size_t i = 0; i < input.size(); ++i)
            input[i] = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1,
                             i == 3   ? 128
                             : i == 0 ? 2
                                      : 1,
                             1, i == 0 ? 2 : 1);
        std::array<Image, 3> output;
        for (auto &target : output)
            target = image(memory, format, VK_IMAGE_TILING_OPTIMAL,
                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, 128, 1, 96);
        // Each slot and each channel has a different value; neutral slots are
        // overridden for the variant under test below.
        std::array<std::array<float, 4>, 20> pixels{};
        for (uint32_t slot = 0; slot != 20; ++slot)
            pixels[slot] = {.03f + slot * .01f, .11f + slot * .012f, .19f + slot * .009f, .27f + slot * .011f};
        pixels[0] = {.25f, .4f, .6f, .8f};
        pixels[1] = {.5f, .63f, .75f, .9f};
        pixels[2] = {.15f, .3f, .45f, .6f};
        pixels[3] = {.2f, .3f, .4f, .5f};
        pixels[4] = {.4f, .2f, .7f, .5f};
        pixels[5] = {.6f, .5f, .2f, .5f};
        pixels[6] = {.8f, .7f, .5f, .5f};
        pixels[7] = {.3f, .4f, .5f, .6f};
        pixels[8] = {.1f, .2f, .7f, .4f};
        for (size_t i = 0; i < input.size(); ++i)
            fill(memory, input[i], pixels[i]);
        const Image material_mips =
            image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1, 4, 3, 4);
        void *material_data{};
        check(vkMapMemory(device, material_mips.memory, 0, VK_WHOLE_SIZE, 0, &material_data));
        for (unsigned mip = 0; mip < 3; ++mip)
        {
            VkImageSubresource sub{VK_IMAGE_ASPECT_COLOR_BIT, mip, 0};
            VkSubresourceLayout sub_layout{};
            vkGetImageSubresourceLayout(device, material_mips.image, &sub, &sub_layout);
            for (unsigned y = 0; y < (4u >> mip); ++y)
                for (unsigned x = 0; x < (4u >> mip); ++x)
                {
                    const float rgba[]{mip == 2 ? .9f : mip == 1 ? .6f : x < 2 ? .2f : .8f, .5f, 1.f, 1.f};
                    std::memcpy(static_cast<uint8_t *>(material_data) + sub_layout.offset + y * sub_layout.rowPitch +
                                    x * 16,
                                rgba, 16);
                }
        }
        vkUnmapMemory(device, material_mips.memory);
        const Image cube = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 6);
        const std::array<float, 4> cube_pixel{.91f, .22f, .34f, .67f};
        fill(memory, cube, cube_pixel, 6);
        VkSampler sampler{};
        VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        check(vkCreateSampler(device, &sampler_info, nullptr, &sampler));
        std::array<VkDescriptorSetLayoutBinding, 20> bindings{};
        for (uint32_t i = 0; i < bindings.size(); ++i)
            bindings[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout_info.bindingCount = bindings.size();
        layout_info.pBindings = bindings.data();
        VkDescriptorSetLayout descriptor_layout{};
        check(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_layout));
        const VkDescriptorPoolSize pool_sizes[]{{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 92},
                                                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4},
                                                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool_info.maxSets = 8;
        pool_info.poolSizeCount = 3;
        pool_info.pPoolSizes = pool_sizes;
        VkDescriptorPool pool{};
        check(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = pool;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &descriptor_layout;
        VkDescriptorSet set{};
        check(vkAllocateDescriptorSets(device, &alloc, &set));
        VkDescriptorSet scene_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &scene_set));
        std::array<VkDescriptorImageInfo, 20> images{};
        std::array<VkWriteDescriptorSet, 20> writes{};
        for (uint32_t i = 0; i < 20; ++i)
        {
            images[i] = {sampler, input[i].view, VK_IMAGE_LAYOUT_GENERAL};
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = set;
            writes[i].dstBinding = i;
            writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &images[i];
        }
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        auto scene_images = images;
        auto scene_writes = writes;
        for (uint32_t i = 0; i < 20; ++i)
        {
            scene_images[i].imageView = input[i == 0 ? 3 : i == 2 ? 19 : i].view;
            scene_writes[i].dstSet = scene_set;
            scene_writes[i].pImageInfo = &scene_images[i];
        }
        vkUpdateDescriptorSets(device, scene_writes.size(), scene_writes.data(), 0, nullptr);
        VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push)};
        VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        const VkDescriptorSetLayout test_layouts[]{descriptor_layout, descriptor_layout};
        pipeline_layout_info.setLayoutCount = 2;
        pipeline_layout_info.pSetLayouts = test_layouts;
        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &range;
        VkPipelineLayout pipeline_layout{};
        check(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));
        const VkDescriptorSetLayoutBinding pose_binding{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                                        VK_SHADER_STAGE_VERTEX_BIT, nullptr};
        layout_info.bindingCount = 1;
        layout_info.pBindings = &pose_binding;
        VkDescriptorSetLayout pose_layout{};
        check(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &pose_layout));
        alloc.pSetLayouts = &pose_layout;
        VkDescriptorSet pose_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &pose_set));
        const VkDescriptorSetLayout skinned_layouts[]{descriptor_layout, pose_layout};
        pipeline_layout_info.pSetLayouts = skinned_layouts;
        VkPipelineLayout skinned_pipeline_layout{};
        check(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &skinned_pipeline_layout));
        const Image floor_depth =
            image(memory, VK_FORMAT_D32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 1, 128, 1, 96);
        std::array<VkAttachmentDescription, 4> attachments{};
        std::array<VkAttachmentReference, 3> references{};
        for (uint32_t i = 0; i < 3; ++i)
        {
            attachments[i].format = format;
            attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            references[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        attachments[3].format = VK_FORMAT_D32_SFLOAT;
        attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        const VkAttachmentReference depth_reference{3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depth_reference;
        subpass.colorAttachmentCount = 3;
        subpass.pColorAttachments = references.data();
        VkRenderPassCreateInfo rp_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp_info.attachmentCount = 4;
        rp_info.pAttachments = attachments.data();
        rp_info.subpassCount = 1;
        rp_info.pSubpasses = &subpass;
        VkSubpassDependency dependencies[2]{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        rp_info.dependencyCount = 2;
        rp_info.pDependencies = dependencies;
        VkRenderPass render_pass{};
        check(vkCreateRenderPass(device, &rp_info, nullptr, &render_pass));
        std::array<VkImageView, 4> views{output[0].view, output[1].view, output[2].view, floor_depth.view};
        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 4;
        framebuffer_info.pAttachments = views.data();
        framebuffer_info.width = 128;
        framebuffer_info.height = 96;
        framebuffer_info.layers = 1;
        VkFramebuffer framebuffer{};
        check(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer));
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = sizeof(Vertex) * 6;
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBuffer vertices{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &vertices));
        VkMemoryRequirements required{};
        vkGetBufferMemoryRequirements(device, vertices, &required);
        VkDeviceMemory vertex_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, vertices, vertex_memory, 0));
        std::array<Vertex, 3> triangle{};
        triangle[0].position[0] = -1;
        triangle[0].position[1] = 1;
        triangle[1].position[0] = 3;
        triangle[1].position[1] = 1;
        triangle[2].position[0] = -1;
        triangle[2].position[1] = -3;
        for (auto &vertex : triangle)
        {
            vertex.normal[2] = 1;
            vertex.static_tangent[0] = 127;
            vertex.static_bitangent[1] = -127;
            vertex.uv[0] = vertex.uv[1] = .5f;
            vertex.lightmap_uv[0] = vertex.lightmap_uv[1] = .5f;
            vertex.baked[0] = .5f;
            vertex.baked[1] = .25f;
            vertex.baked[2] = .75f;
            vertex.baked[3] = 1.f;
        }
        // Match the prepared mesh tangent frame to these UVs, independently
        // of screen derivatives and camera angle.
        triangle[0].uv[0] = 0;
        triangle[0].uv[1] = 0;
        triangle[1].uv[0] = 2;
        triangle[1].uv[1] = 0;
        triangle[2].uv[0] = 0;
        triangle[2].uv[1] = 2;
        void *mapped{};
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        buffer_info.size = 128;
        buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        VkBuffer pose_buffer{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &pose_buffer));
        vkGetBufferMemoryRequirements(device, pose_buffer, &required);
        auto pose_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, pose_buffer, pose_memory, 0));
        std::array<float, 32> pose_data{};
        for (unsigned i = 0; i < 16; i += 5)
            pose_data[i] = 1;
        const float object_rows[]{1, 0, 0, -1, 0, 1, 0, 0, 0, 0, 1, 0};
        auto object_data = xray::render::vulkan::model_pose_constants(object_rows, nullptr);
        std::copy(object_data.begin(), object_data.end(), pose_data.begin() + 16);
        auto write_pose = [&]() {
            void *data{};
            check(vkMapMemory(device, pose_memory, 0, VK_WHOLE_SIZE, 0, &data));
            std::memcpy(data, pose_data.data(), sizeof(pose_data));
            vkUnmapMemory(device, pose_memory);
        };
        write_pose();
        const VkDescriptorBufferInfo pose_info{pose_buffer, 0, 128};
        VkWriteDescriptorSet pose_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        pose_write.dstSet = pose_set;
        pose_write.descriptorCount = 1;
        pose_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        pose_write.pBufferInfo = &pose_info;
        vkUpdateDescriptorSets(device, 1, &pose_write, 0, nullptr);
        buffer_info.size = 128 * 96 * 3 * 16;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VkBuffer readback{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &readback));
        vkGetBufferMemoryRequirements(device, readback, &required);
        VkDeviceMemory readback_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, readback, readback_memory, 0));
        VkVertexInputBindingDescription vertex_binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[]{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
            {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
            {5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, lightmap_uv)},
            {6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, baked)},
            {7, 0, VK_FORMAT_R8G8B8A8_SNORM, offsetof(Vertex, static_tangent)},
            {8, 0, VK_FORMAT_R8G8B8A8_SNORM, offsetof(Vertex, static_bitangent)}};
        const VkVertexInputAttributeDescription skinned_attributes[]{
            attributes[0],
            attributes[1],
            attributes[2],
            {3, 0, VK_FORMAT_R16G16B16A16_UINT, offsetof(Vertex, bones)},
            {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weights)},
            {7, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
            {8, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, bitangent)}};
        VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex_input.vertexBindingDescriptionCount = 1;
        vertex_input.pVertexBindingDescriptions = &vertex_binding;
        vertex_input.vertexAttributeDescriptionCount = 7;
        vertex_input.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0, 0, 1, 1, 0, 1};
        VkRect2D scissor{{0, 0}, {1, 1}};
        VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport_state.viewportCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1;
        viewport_state.pScissors = &scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend_attachment{};
        blend_attachment.colorWriteMask = 15;
        std::array<VkPipelineColorBlendAttachmentState, 3> color_blends{};
        color_blends.fill(blend_attachment);
        VkPipelineColorBlendStateCreateInfo blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend_state.attachmentCount = 3;
        blend_state.pAttachments = color_blends.data();
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline_info.pVertexInputState = &vertex_input;
        pipeline_info.pInputAssemblyState = &assembly;
        pipeline_info.pViewportState = &viewport_state;
        pipeline_info.pRasterizationState = &raster;
        pipeline_info.pMultisampleState = &multisample;
        pipeline_info.pDepthStencilState = &depth;
        pipeline_info.pColorBlendState = &blend_state;
        pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = render_pass;
        VkCommandPoolCreateInfo cmd_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        cmd_pool_info.queueFamilyIndex = family;
        VkCommandPool commands{};
        check(vkCreateCommandPool(device, &cmd_pool_info, nullptr, &commands));
        VkCommandBufferAllocateInfo cmd_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmd_info.commandPool = commands;
        cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_info.commandBufferCount = 1;
        VkCommandBuffer cmd{};
        check(vkAllocateCommandBuffers(device, &cmd_info, &cmd));
        auto module = [&](const std::string &filename) {
            auto code = shader(std::string(argv[1]) + "/" + filename);
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            info.codeSize = code.size() * 4;
            info.pCode = code.data();
            VkShaderModule result{};
            check(vkCreateShaderModule(device, &info, nullptr, &result));
            return result;
        };
        const auto vs = module("level_terrain.vs.spv");
        const auto terrain = module("level_terrain.ps.spv");
        const auto detail = module("level_detail.ps.spv");
        const auto flat = module("level_lightmap.ps.spv");
        const auto lod_vertex = module("level_lod.vs.spv");
        const auto lod_fragment = module("level_lod.ps.spv");
        const auto laemb = module("level_laemb_base.ps.spv");
        const auto reflection = module("level_environment.ps.spv");
        const auto deferred_particle = module("particle_world_deferred.ps.spv");
        const auto additive_particle = module("particle_world_additive.ps.spv");
        const auto additive_world_set = module("world_set_additive.ps.spv");
        auto embedded = [&](const uint32_t *code, size_t bytes) {
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            info.codeSize = bytes;
            info.pCode = code;
            VkShaderModule result{};
            check(vkCreateShaderModule(device, &info, nullptr, &result));
            return result;
        };
        using namespace xray::render::vulkan;
        const auto distort_vs = embedded(distortion_shaders::Vertex, sizeof(distortion_shaders::Vertex));
        const auto distort_ps = embedded(distortion_shaders::Fragment, sizeof(distortion_shaders::Fragment));
        const auto ui_vs = embedded(scene_shaders::UiVertex, sizeof(scene_shaders::UiVertex));
        const auto ui_ps = embedded(scene_shaders::UiFragment, sizeof(scene_shaders::UiFragment));
        std::array<std::array<float, 4>, 128> last_ui_pixels{};
        std::array<std::array<float, 4>, 128 * 96> scene_pixels{};
        bool scene_test = false;
        bool distortion_mask = false;
        std::array<std::array<float, 4>, 4> sprite_pixels{};
        const auto weather_ps = embedded(weather_shaders::Fragment, sizeof(weather_shaders::Fragment));
        const auto weather_vs = embedded(deferred_shaders::LightVertex, sizeof(deferred_shaders::LightVertex));
        const Image weather_normal =
            image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1, 128);
        const Image weather_shadow =
            image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1, 128);
        const Image weather_cube =
            image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 6, 64, 7);
        // Deliberately different mip contents expose implicit-LOD selection.
        check(vkMapMemory(device, weather_cube.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        for (unsigned face = 0; face < 6; ++face)
            for (unsigned mip = 0; mip < 7; ++mip)
            {
                VkImageSubresource sub{VK_IMAGE_ASPECT_COLOR_BIT, mip, face};
                VkSubresourceLayout layout{};
                vkGetImageSubresourceLayout(device, weather_cube.image, &sub, &layout);
                const float rgba[]{mip ? 0.f : .8f, mip ? 0.f : .6f, mip ? 0.f : .4f, 1.f};
                for (unsigned y = 0; y < (64u >> mip); ++y)
                    for (unsigned x = 0; x < (64u >> mip); ++x)
                        std::memcpy(static_cast<uint8_t *>(mapped) + layout.offset + y * layout.rowPitch + x * 16, rgba,
                                    16);
            }
        vkUnmapMemory(device, weather_cube.memory);
        check(vkMapMemory(device, weather_normal.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        VkImageSubresource normal_sub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout normal_layout{};
        vkGetImageSubresourceLayout(device, weather_normal.image, &normal_sub, &normal_layout);
        for (unsigned x = 0; x < 128; ++x)
        {
            const float rgba[]{x % 2 ? .9f : .1f, .8f, .5f, .75f};
            std::memcpy(static_cast<uint8_t *>(mapped) + normal_layout.offset + x * 16, rgba, 16);
        }
        vkUnmapMemory(device, weather_normal.memory);
        check(vkMapMemory(device, weather_shadow.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        VkSubresourceLayout shadow_layout{};
        vkGetImageSubresourceLayout(device, weather_shadow.image, &normal_sub, &shadow_layout);
        for (unsigned x = 0; x < 128; ++x)
        {
            const float rgba[]{x % 2 ? .75f : .25f, 0, 0, 1};
            std::memcpy(static_cast<uint8_t *>(mapped) + shadow_layout.offset + x * 16, rgba, 16);
        }
        vkUnmapMemory(device, weather_shadow.memory);
        VkBuffer weather_uniform{};
        buffer_info.size = 176;
        buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        check(vkCreateBuffer(device, &buffer_info, nullptr, &weather_uniform));
        vkGetBufferMemoryRequirements(device, weather_uniform, &required);
        auto weather_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, weather_uniform, weather_memory, 0));
        float constants[44]{};
        for (unsigned i = 0; i < 16; ++i)
            constants[i] = constants[16 + i] = i % 5 == 0 ? 1.f : 0.f;
        constants[36] = constants[37] = constants[38] = .04f;
        constants[40] = .5f;
        constants[41] = .45f;
        constants[42] = .4f;
        check(vkMapMemory(device, weather_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, constants, sizeof(constants));
        vkUnmapMemory(device, weather_memory);
        auto weather_bindings = bindings;
        weather_bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layout_info.pBindings = weather_bindings.data();
        layout_info.bindingCount = 6;
        VkDescriptorSetLayout weather_descriptor_layout{};
        check(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &weather_descriptor_layout));
        alloc.pSetLayouts = &weather_descriptor_layout;
        VkDescriptorSet weather_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &weather_set));
        alloc.pSetLayouts = &descriptor_layout;
        VkDescriptorSet weather_environment_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &weather_environment_set));
        VkDescriptorImageInfo weather_images[]{{sampler, input[0].view, VK_IMAGE_LAYOUT_GENERAL},
                                               {sampler, weather_normal.view, VK_IMAGE_LAYOUT_GENERAL},
                                               {sampler, input[19].view, VK_IMAGE_LAYOUT_GENERAL},
                                               {sampler, weather_shadow.view, VK_IMAGE_LAYOUT_GENERAL},
                                               {},
                                               {sampler, input[2].view, VK_IMAGE_LAYOUT_GENERAL}};
        VkDescriptorBufferInfo uniform_info{weather_uniform, 0, 176};
        for (unsigned i = 0; i < 6; ++i)
        {
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = weather_set;
            write.dstBinding = i;
            write.descriptorCount = 1;
            write.descriptorType =
                i == 4 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &weather_images[i];
            if (i == 4)
                write.pBufferInfo = &uniform_info;
            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
            VkDescriptorImageInfo cube_info{sampler, weather_cube.view, VK_IMAGE_LAYOUT_GENERAL};
            write.dstSet = weather_environment_set;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = i == 2 || i == 3 ? &weather_images[0] : &cube_info;
            write.pBufferInfo = nullptr;
            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }
        const VkDescriptorSetLayout weather_layouts[]{weather_descriptor_layout, descriptor_layout};
        const VkPushConstantRange weather_range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 128};
        pipeline_layout_info.pSetLayouts = weather_layouts;
        pipeline_layout_info.pPushConstantRanges = &weather_range;
        VkPipelineLayout weather_pipeline_layout{};
        check(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &weather_pipeline_layout));
        bool weather_initialized = false;
        const auto local_fragment = embedded(local_light_shaders::Fragment, sizeof(local_light_shaders::Fragment));
        const VkDescriptorSetLayoutBinding local_bindings[]{
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
            {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        layout_info.bindingCount = 3;
        layout_info.pBindings = local_bindings;
        VkDescriptorSetLayout local_layout{};
        check(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &local_layout));
        alloc.pSetLayouts = &local_layout;
        VkDescriptorSet local_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &local_set));
        const VkDescriptorSetLayout local_layouts[]{descriptor_layout, local_layout};
        pipeline_layout_info.pSetLayouts = local_layouts;
        pipeline_layout_info.pPushConstantRanges = &range;
        VkPipelineLayout local_pipeline_layout{};
        check(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &local_pipeline_layout));
        VkImageViewCreateInfo array_view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        array_view_info.image = cube.image;
        array_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        array_view_info.format = format;
        array_view_info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
        VkImageView shadow_array_view{};
        check(vkCreateImageView(device, &array_view_info, nullptr, &shadow_array_view));
        buffer_info.size = sizeof(LocalLightUniform);
        buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkBuffer local_buffer{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &local_buffer));
        vkGetBufferMemoryRequirements(device, local_buffer, &required);
        auto local_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, local_buffer, local_memory, 0));
        LocalLightUniform local_uniform{};
        auto write_local = [&]() {
            void *value{};
            check(vkMapMemory(device, local_memory, 0, VK_WHOLE_SIZE, 0, &value));
            std::memcpy(value, &local_uniform, sizeof(local_uniform));
            vkUnmapMemory(device, local_memory);
        };
        const VkDescriptorImageInfo local_images[]{{sampler, shadow_array_view, VK_IMAGE_LAYOUT_GENERAL},
                                                   {sampler, input[18].view, VK_IMAGE_LAYOUT_GENERAL}};
        const VkDescriptorBufferInfo local_buffer_info{local_buffer, 0, sizeof(LocalLightUniform)};
        VkWriteDescriptorSet local_writes[3]{};
        for (unsigned i = 0; i < 3; ++i)
        {
            auto &write = local_writes[i];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = local_set;
            write.dstBinding = i;
            write.descriptorCount = 1;
            write.descriptorType =
                i == 1 ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            if (i == 1)
                write.pBufferInfo = &local_buffer_info;
            else
                write.pImageInfo = &local_images[i == 0 ? 0 : 1];
        }
        vkUpdateDescriptorSets(device, 3, local_writes, 0, nullptr);
        const auto instanced_vertex = module("level_detail_instanced.vs.spv");
        const DetailInstance detail_instances[]{detail_instance(-.5f, 0, .5f, 0, .6f),
                                                detail_instance(.5f, 0, .5f, 3.14159265359f, .6f)};
        buffer_info.size = sizeof(detail_instances);
        buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBuffer instance_buffer{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &instance_buffer));
        vkGetBufferMemoryRequirements(device, instance_buffer, &required);
        auto instance_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, instance_buffer, instance_memory, 0));
        check(vkMapMemory(device, instance_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, detail_instances, sizeof(detail_instances));
        vkUnmapMemory(device, instance_memory);
        const VkVertexInputBindingDescription instance_bindings[]{
            vertex_binding, {1, sizeof(DetailInstance), VK_VERTEX_INPUT_RATE_INSTANCE}};
        const VkVertexInputAttributeDescription instance_attributes[]{
            attributes[0],
            attributes[1],
            attributes[2],
            attributes[3],
            attributes[4],
            attributes[5],
            attributes[6],
            {9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(DetailInstance, position_scale)},
            {10, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(DetailInstance, rotation)}};
        bool inputs_initialized = false;
        const auto forward_fragment = module("object_priority_opaque.ps.spv");
        const auto forward_vertex = module("object_opaque.vs.spv");
        ForwardLightUniform forward_data{};
        for (unsigned i = 0; i < 16; i += 5)
            forward_data.inverse_view_projection[i] = 1;
        forward_data.sun_direction_ambient[2] = -1;
        forward_data.sun_color_count[3] = ForwardLightCapacity;
        forward_data.viewport[0] = forward_data.viewport[1] = 1;
        for (unsigned i : {32u, 63u})
        {
            forward_data.local[i].position_range[2] = 2;
            forward_data.local[i].position_range[3] = 10;
        }
        forward_data.local[32].color_type[0] = 1;
        forward_data.local[63].color_type[1] = 1;
        buffer_info.size = sizeof(forward_data);
        buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        VkBuffer forward_buffer{};
        check(vkCreateBuffer(device, &buffer_info, nullptr, &forward_buffer));
        vkGetBufferMemoryRequirements(device, forward_buffer, &required);
        auto forward_memory =
            memory.allocate(required, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, forward_buffer, forward_memory, 0));
        check(vkMapMemory(device, forward_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, &forward_data, sizeof(forward_data));
        vkUnmapMemory(device, forward_memory);
        const VkDescriptorSetLayoutBinding forward_binding{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                                           VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo forward_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        forward_layout_info.bindingCount = 1;
        forward_layout_info.pBindings = &forward_binding;
        VkDescriptorSetLayout forward_descriptor{};
        check(vkCreateDescriptorSetLayout(device, &forward_layout_info, nullptr, &forward_descriptor));
        auto forward_allocation = alloc;
        forward_allocation.pSetLayouts = &forward_descriptor;
        VkDescriptorSet forward_set{};
        check(vkAllocateDescriptorSets(device, &forward_allocation, &forward_set));
        VkDescriptorBufferInfo forward_info{forward_buffer, 0, sizeof(forward_data)};
        VkWriteDescriptorSet forward_write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        forward_write.dstSet = forward_set;
        forward_write.dstBinding = 0;
        forward_write.descriptorCount = 1;
        forward_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        forward_write.pBufferInfo = &forward_info;
        vkUpdateDescriptorSets(device, 1, &forward_write, 0, nullptr);
        const VkDescriptorSetLayout forward_layouts[]{descriptor_layout, forward_descriptor, descriptor_layout};
        auto forward_pipeline_info = pipeline_layout_info;
        forward_pipeline_info.setLayoutCount = 3;
        forward_pipeline_info.pSetLayouts = forward_layouts;
        forward_pipeline_info.pPushConstantRanges = &range;
        VkPipelineLayout forward_pipeline_layout{};
        check(vkCreatePipelineLayout(device, &forward_pipeline_info, nullptr, &forward_pipeline_layout));
        VkDescriptorSetLayoutBinding composite_bindings[]{bindings[0], bindings[1], bindings[2],
            {3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo composite_layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        composite_layout_info.bindingCount = 4;
        composite_layout_info.pBindings = composite_bindings;
        VkDescriptorSetLayout composite_descriptor{};
        check(vkCreateDescriptorSetLayout(device, &composite_layout_info, nullptr, &composite_descriptor));
        alloc.pSetLayouts = &composite_descriptor;
        VkDescriptorSet composite_set{};
        check(vkAllocateDescriptorSets(device, &alloc, &composite_set));
        const VkDescriptorSetLayout composite_layouts[]{descriptor_layout, composite_descriptor};
        auto composite_pipeline_info = pipeline_layout_info;
        composite_pipeline_info.setLayoutCount = 2;
        composite_pipeline_info.pSetLayouts = composite_layouts;
        composite_pipeline_info.pPushConstantRanges = &range;
        VkPipelineLayout composite_pipeline_layout{};
        check(vkCreatePipelineLayout(device, &composite_pipeline_info, nullptr, &composite_pipeline_layout));
        const auto composite_ps = embedded(distortion_shaders::Composite, sizeof(distortion_shaders::Composite));
        const auto water_ps = embedded(water_shaders::Fragment, sizeof(water_shaders::Fragment));
        const auto water_vs = embedded(deferred_shaders::GBufferVertex, sizeof(deferred_shaders::GBufferVertex));
        bool cube_initialized = false;
        bool material_mips_initialized = false;
        auto execute = [&](VkShaderModule fragment, const Push &push, bool use_cube = false,
                           VkShaderModule custom_vertex = VK_NULL_HANDLE, bool sprite = false, bool skinned = false,
                           bool coplanar = false, bool multiply_decal = false, bool cutout_bias = false) {
            VkPipelineShaderStageCreateInfo stages[2]{};
            for (auto &stage : stages)
                stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
            stages[0].module = custom_vertex ? custom_vertex : vs;
            stages[0].pName = "main";
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            stages[1].module = fragment;
            stages[1].pName = "main";
            pipeline_info.stageCount = 2;
            pipeline_info.pStages = stages;
            const VkVertexInputAttributeDescription ui_attributes[]{
                {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)},
                {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, baked)},
                {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, clip_depth_w)}};
            const bool ui = custom_vertex == ui_vs;
            const bool weather = fragment == weather_ps;
            const bool local = fragment == local_fragment;
            const bool forward = fragment == forward_fragment;
            const bool instanced = custom_vertex == instanced_vertex;
            const bool wide = ui || weather || scene_test;
            viewport.width = wide ? 128.f : sprite ? 2.f : 1.f;
            scissor.extent.width = wide ? 128 : sprite ? 2 : 1;
            viewport.height = scene_test ? 96.f : sprite ? 2.f : 1.f;
            scissor.extent.height = scene_test ? 96 : sprite ? 2 : 1;
            pipeline_info.layout = (fragment == composite_ps || fragment == water_ps) ? composite_pipeline_layout
                                   : forward   ? forward_pipeline_layout
                                   : local   ? local_pipeline_layout
                                   : weather ? weather_pipeline_layout
                                   : skinned ? skinned_pipeline_layout
                                             : pipeline_layout;
            vertex_input.vertexAttributeDescriptionCount = instanced ? 9 : ui ? 4 : 7;
            vertex_input.vertexBindingDescriptionCount = instanced ? 2 : 1;
            vertex_input.pVertexBindingDescriptions = instanced ? instance_bindings : &vertex_binding;
            vertex_input.pVertexAttributeDescriptions = instanced ? instance_attributes
                                                        : ui      ? ui_attributes
                                                        : skinned ? skinned_attributes
                                                                  : attributes;
            depth.depthTestEnable = coplanar;
            depth.depthWriteEnable = coplanar;
            depth.depthCompareOp = xray::render::vulkan::geometry_depth_compare(false);
            const auto bias = xray::render::vulkan::geometry_depth_bias(false, false, cutout_bias);
            raster.depthBiasEnable = bias.enabled;
            raster.depthBiasConstantFactor = bias.constant;
            raster.depthBiasSlopeFactor = bias.slope;
            color_blends[0].blendEnable = multiply_decal || local || distortion_mask;
            color_blends[0].colorWriteMask = multiply_decal ? 7 : 15;
            color_blends[0].srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            color_blends[0].dstColorBlendFactor = local ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_COLOR;
            if (local)
                color_blends[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            if (distortion_mask)
            {
                color_blends[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                color_blends[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            }
            VkPipeline pipeline{};
            check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));
            check(vkResetCommandPool(device, commands, 0));
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            check(vkBeginCommandBuffer(cmd, &begin));
            if ((weather || forward) && !weather_initialized)
            {
                VkImageMemoryBarrier barriers[3]{};
                for (unsigned i = 0; i < 3; ++i)
                {
                    auto &b = barriers[i];
                    b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    b.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                    b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    b.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                    b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    b.image = i == 0 ? weather_normal.image : i == 1 ? weather_cube.image : weather_shadow.image;
                    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, i == 1 ? 7u : 1u, 0, i == 1 ? 6u : 1u};
                    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                }
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 3, barriers);
                weather_initialized = true;
            }
            if ((use_cube || local) && !cube_initialized)
            {
                VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                barrier.image = cube.image;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.layerCount = 6;
                barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &barrier);
                cube_initialized = true;
            }
            {
                std::array<VkImageMemoryBarrier, 20> barriers{};
                for (size_t i = 0; i < barriers.size(); ++i)
                {
                    barriers[i] = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                    barriers[i].oldLayout =
                        inputs_initialized ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_PREINITIALIZED;
                    barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    barriers[i].image = input[i].image;
                    barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barriers[i].subresourceRange.levelCount = barriers[i].subresourceRange.layerCount = 1;
                    barriers[i].srcQueueFamilyIndex = barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barriers[i].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                    barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                }
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                     nullptr, 0, nullptr, barriers.size(), barriers.data());
                inputs_initialized = true;
            }
            if (!material_mips_initialized)
            {
                VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                b.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                b.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                b.image = material_mips.image;
                b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 3, 0, 1};
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                                     nullptr, 0, nullptr, 1, &b);
                material_mips_initialized = true;
            }
            std::array<VkClearValue, 4> clear{};
            clear[3].depthStencil = {cutout_bias ? .5f : coplanar ? 0.f : 1.f, 0};
            if (multiply_decal)
                clear[0].color = {{.2f, .3f, .4f, 1.f}};
            if (local)
                clear[0].color = {{.1f, .1f, .1f, 1.f}};
            if (distortion_mask)
                clear[0].color = {{127.f / 255.f, 127.f / 255.f, 0.f, 127.f / 255.f}};
            VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            rp.renderPass = render_pass;
            rp.framebuffer = framebuffer;
            rp.renderArea.extent = {1, 1};
            rp.renderArea.extent.width = wide ? 128 : sprite ? 2 : 1;
            rp.renderArea.extent.height = scene_test ? 96 : sprite ? 2 : 1;
            rp.clearValueCount = 4;
            rp.pClearValues = clear.data();
            vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            const VkDescriptorSet draw_sets[]{weather ? weather_set : set, local     ? local_set
                                                                           : weather ? weather_environment_set
                                                                           : skinned ? pose_set
                                                                           : (fragment == composite_ps || fragment == water_ps) ? composite_set
                                                                                     : scene_set};
            const VkDescriptorSet forward_sets[]{set, forward_set, weather_environment_set};
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_info.layout, 0, forward ? 3 : 2,
                                    forward ? forward_sets : draw_sets, 0, nullptr);
            VkDeviceSize offset{};
            vkCmdBindVertexBuffers(cmd, 0, 1, &vertices, &offset);
            vkCmdPushConstants(cmd, pipeline_info.layout, range.stageFlags, 0, weather ? 128 : sizeof(push), &push);
            if (instanced)
                vkCmdBindVertexBuffers(cmd, 1, 1, &instance_buffer, &offset);
            vkCmdDraw(cmd, sprite ? 6 : 3, instanced ? 2 : 1, 0, 0);
            vkCmdEndRenderPass(cmd);
            std::array<VkBufferImageCopy, 3> copies{};
            for (uint32_t i = 0; i < 3; ++i)
            {
                copies[i].bufferOffset = i * (128 * 96) * 16;
                copies[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copies[i].imageSubresource.layerCount = 1;
                copies[i].imageExtent = {wide ? 128u : sprite ? 2u : 1u, scene_test ? 96u : sprite ? 2u : 1u, 1};
            }
            for (uint32_t i = 0; i < 3; ++i)
                vkCmdCopyImageToBuffer(cmd, output[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback, 1,
                                       &copies[i]);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            host.buffer = readback;
            host.size = 128 * 96 * 48;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1,
                                 &host, 0, nullptr);
            check(vkEndCommandBuffer(cmd));
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &cmd;
            check(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE));
            check(vkQueueWaitIdle(queue));
            std::array<std::array<float, 4>, 3> result{};
            void *data{};
            check(vkMapMemory(device, readback_memory, 0, VK_WHOLE_SIZE, 0, &data));
            for (unsigned i = 0; i < 3; ++i)
                std::memcpy(result[i].data(), static_cast<uint8_t *>(data) + i * (128 * 96) * 16, 16);
            if (scene_test)
                std::memcpy(scene_pixels.data(), data, sizeof(scene_pixels));
            if (wide)
                std::memcpy(last_ui_pixels.data(), data, sizeof(last_ui_pixels));
            if (sprite)
                std::memcpy(sprite_pixels.data(), data, sizeof(sprite_pixels));
            vkUnmapMemory(device, readback_memory);
            vkDestroyPipeline(device, pipeline, nullptr);
            return result;
        };
        Push push{};
        for (int i = 0; i < 16; ++i)
            push.mvp[i] = (i % 5 == 0 ? 1.f : 0.f);
        for (int index : {0, 5, 8, 13, 16, 21, 26})
            push.matrices[index] = 1.f;
        push.scale = 1;
        push.modes[3] = 1;
        push.factors[1] = 1;
        const auto forward_color = execute(forward_fragment, push, false, forward_vertex);
        near(forward_color[0][0], pixels[0][0] * .96f,
             "forward object receives light 32 beyond old camera-dependent first-16 cutoff");
        near(forward_color[0][1], pixels[0][1] * .96f, "forward object receives light 63");
        near(forward_color[0][2], 0.f, "forward object unlit channel");
        // Binding 1 deliberately clamps lightmaps to mip zero; base and normal
        // textures must still repeat and select coarse mips with distance.
        {
            const auto saved_triangle = triangle;
            const auto saved_images = images;
            auto sampler_config = sampler_info;
            sampler_config.maxLod = VK_LOD_CLAMP_NONE;
            VkSampler material_sampler{};
            check(vkCreateSampler(device, &sampler_config, nullptr, &material_sampler));
            sampler_config.maxLod = 0;
            sampler_config.addressModeU = sampler_config.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VkSampler lightmap_sampler{};
            check(vkCreateSampler(device, &sampler_config, nullptr, &lightmap_sampler));
            images[0] = {material_sampler, material_mips.view, VK_IMAGE_LAYOUT_GENERAL};
            images[1].sampler = lightmap_sampler;
            images[15] = {material_sampler, material_mips.view, VK_IMAGE_LAYOUT_GENERAL};
            for (unsigned i : {3u, 4u, 5u, 6u})
                images[i] = {material_sampler, material_mips.view, VK_IMAGE_LAYOUT_GENERAL};
            for (unsigned i : {0u, 1u, 3u, 4u, 5u, 6u, 15u})
                vkUpdateDescriptorSets(device, 1, &writes[i], 0, nullptr);
            for (auto &v : triangle)
                v.uv[0] = 1.125f, v.uv[1] = .125f;
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
            near(execute(flat, push)[0][0], .2f, "base texture repeats independently of clamped lightmap");
            near(execute(terrain, push)[0][0], .08f,
                 "terrain base and four detail maps repeat instead of sampling black borders");
            triangle = saved_triangle;
            for (auto &v : triangle)
            {
                v.uv[0] *= 8;
                v.uv[1] *= 8;
            }
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
            near(execute(flat, push)[0][0], .9f, "base texture uses coarse mip independently of lightmap mip zero");
            auto normal_push = push;
            normal_push.modes[2] = 1;
            if (execute(flat, normal_push)[1][0] < .6f)
                throw std::runtime_error("normal map incorrectly sampled lightmap mip zero");
            images = saved_images;
            for (unsigned i : {0u, 1u, 3u, 4u, 5u, 6u, 15u})
                vkUpdateDescriptorSets(device, 1, &writes[i], 0, nullptr);
            triangle = saved_triangle;
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
            vkDestroySampler(device, material_sampler, nullptr);
            vkDestroySampler(device, lightmap_sampler, nullptr);
        }
        // Singular level frames use the uploaded geometric normal rather
        // than a UV derivative frame that changes with the view projection.
        const auto prepared_triangle = triangle;
        for (auto &v : triangle)
            for (unsigned axis = 0; axis < 4; ++axis)
                v.static_tangent[axis] = v.static_bitangent[axis] = 0;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        auto level_normal_push = push;
        level_normal_push.modes[2] = 1;
        for (float angle : {-.4f, 0.f, .4f})
        {
            auto camera = level_normal_push;
            camera.mvp[0] = camera.mvp[10] = std::cos(angle);
            camera.mvp[2] = -std::sin(angle);
            camera.mvp[8] = std::sin(angle);
            camera.mvp[14] = .5f;
            auto level = execute(flat, camera);
            near(level[1][0], .5f, "singular level normal X is camera independent");
            near(level[1][1], .5f, "singular level normal Y is camera independent");
            near(level[1][2], 1.f, "singular level normal Z is camera independent");
        }
        triangle = prepared_triangle;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        // An authored frame rotated relative to the UV gradients must survive
        // GPU skinning, world rotation and changes to fragment material data.
        auto character_push = push;
        character_push.modes[0] = 0;
        character_push.modes[2] = 1;
        const auto saved_normal = pixels[15];
        fill(memory, input[15], {.8f, .5f, .9f, 1});
        auto character_fragment = module("skinned_opaque.ps.spv");
        for (unsigned links = 1; links <= 4; ++links)
        {
            auto character_vertex = module("skinned_" + std::to_string(links) + ".vs.spv");
            auto character = execute(character_fragment, character_push, false, character_vertex, false, true);
            near(character[1][0], .5f, "authored character tangent X");
            near(character[1][1], .8f, "authored character tangent Y");
            near(character[1][2], .9f, "authored character tangent Z");
            character_push.modes[3] = 2;
            character_push.factors[0] = .7f;
            auto changed = execute(character_fragment, character_push, false, character_vertex, false, true);
            for (unsigned c = 0; c < 3; ++c)
                near(changed[1][c], character[1][c], "material constants do not overwrite model transform");
            vkDestroyShaderModule(device, character_vertex, nullptr);
        }
        // Singular UV frames keep a geometric normal in every skin mode.
        const auto authored_triangle = triangle;
        for (auto &v : triangle)
            for (unsigned axis = 0; axis < 3; ++axis)
                v.tangent[axis] = v.bitangent[axis] = 0;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        for (unsigned links = 1; links <= 4; ++links)
        {
            auto vertex = module("skinned_" + std::to_string(links) + ".vs.spv");
            auto fallback = execute(character_fragment, character_push, false, vertex, false, true);
            near(fallback[1][0], .5f, "missing frame geometric X");
            near(fallback[1][1], .5f, "missing frame geometric Y");
            near(fallback[1][2], 1.f, "missing frame geometric Z");
            for (float angle : {-.4f, .2f, .5f})
            {
                auto camera = character_push;
                camera.mvp[0] = camera.mvp[5] = std::cos(angle);
                camera.mvp[1] = std::sin(angle);
                camera.mvp[4] = -std::sin(angle);
                auto turned = execute(character_fragment, camera, false, vertex, false, true);
                for (unsigned axis = 0; axis < 3; ++axis)
                    near(turned[1][axis], fallback[1][axis], "singular model normal is camera independent");
            }
            vkDestroyShaderModule(device, vertex, nullptr);
        }
        for (auto &v : triangle)
        {
            v.tangent[0] = v.tangent[1] = v.bitangent[0] = v.bitangent[1] = 0;
            v.tangent[2] = 1;
            v.bitangent[2] = 2;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        for (unsigned links = 1; links <= 4; ++links)
        {
            auto vertex = module("skinned_" + std::to_string(links) + ".vs.spv");
            auto fallback = execute(character_fragment, character_push, false, vertex, false, true);
            near(fallback[1][0], .5f, "collapsed frame keeps geometric X");
            near(fallback[1][2], 1.f, "collapsed frame keeps geometric Z");
            vkDestroyShaderModule(device, vertex, nullptr);
        }
        triangle = authored_triangle;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        const float rotated_rows[]{0, 0, 1, -1, 0, 1, 0, 0, -1, 0, 0, 0};
        object_data = model_pose_constants(rotated_rows, nullptr);
        std::copy(object_data.begin(), object_data.end(), pose_data.begin() + 16);
        write_pose();
        auto character_vertex = module("skinned_1.vs.spv");
        auto rotated = execute(character_fragment, character_push, false, character_vertex, false, true);
        near(rotated[1][0], .9f, "character normal world rotation X");
        near(rotated[1][1], .8f, "character normal world rotation Y");
        near(rotated[1][2], .5f, "character normal world rotation Z");
        vkDestroyShaderModule(device, character_vertex, nullptr);
        vkDestroyShaderModule(device, character_fragment, nullptr);
        fill(memory, input[15], saved_normal);
        // Forward decal at exactly the stored floor depth. LESS would reject
        // every pixel, leaving the clear color instead of the paper texture.
        auto paper_fragment = module("world_set_blended.ps.spv");
        auto paper = execute(paper_fragment, push, false, VK_NULL_HANDLE, false, false, true);
        near(paper[0][0], pixels[0][0] * triangle[0].baked[0], "coplanar paper decal survives floor depth");
        vkDestroyShaderModule(device, paper_fragment, nullptr);
        // Wallmark Lua uses simple.ps: zero baked COLOR must not erase alpha.
        const auto paper_vertices = triangle;
        for (auto &v : triangle)
            std::fill_n(v.baked, 4, 0.f);
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        auto generic_paper = module("world_set_blended.ps.spv");
        auto hidden_paper = execute(generic_paper, push, false, VK_NULL_HANDLE, false, false, true);
        near(hidden_paper[0][0], 0.f, "old generic SET erases the zero-COLOR decal");
        vkDestroyShaderModule(device, generic_paper, nullptr);
        auto simple_paper = module("wallmark_blended.ps.spv");
        auto visible_paper = execute(simple_paper, push, false, VK_NULL_HANDLE, false, false, true);
        near(visible_paper[0][0], pixels[0][0], "wallmark ignores zero baked RGB and alpha");
        vkDestroyShaderModule(device, simple_paper, nullptr);
        triangle = paper_vertices;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        // Static model rotation also lives outside fragment material bytes.
        auto object_vertex = module("object_opaque.vs.spv"), object_fragment = module("object_opaque.ps.spv");
        auto object_push = push;
        const auto object_quaternion = model_rotation_quaternion(rotated_rows);
        std::copy(object_quaternion.begin(), object_quaternion.end(), object_push.rotation);
        object_push.matrices[2] = pack_model_hemi(.2f, .8f);
        object_push.matrices[6] = pack_model_hemi(.4f, .6f);
        object_push.matrices[10] = pack_model_hemi(.5f, .7f);
        auto object_result = execute(object_fragment, object_push, false, object_vertex);
        near(object_result[1][3], .875f + .125f * .2f, "packed hemi survives static rotation/material block");
        near(object_result[1][0], 1.f, "static model normal rotation X");
        near(object_result[1][2], .5f, "static model normal rotation Z");
        object_push.modes[3] = 3;
        object_push.factors[0] = .8f;
        auto object_changed = execute(object_fragment, object_push, false, object_vertex);
        for (unsigned c = 0; c < 3; ++c)
            near(object_changed[1][c], object_result[1][c], "static material data cannot alter rotation");
        // Feed an authored build.lights record through the real object shader.
        // Camera depth must not remove the same object's local hemi term.
        std::array<uint8_t, 104> baked_light{};
        const auto put_float = [&](size_t offset, float value) { std::memcpy(baked_light.data() + offset, &value, 4); };
        baked_light[0] = 1;
        for (unsigned axis = 0; axis < 3; ++axis)
            put_float(4 + 4 * axis, 1);
        put_float(16, 5);
        put_float(40, 10);
        put_float(48, .01f);
        put_float(52, 2);
        put_float(56, .1f);
        std::vector<xray::render::vulkan::VulkanLightSnapshot> baked_sources;
        std::string baked_error;
        if (!xray::render::vulkan::decode_level_lights({baked_light.data(), baked_light.size()}, true, baked_sources,
                                                       baked_error))
            throw std::runtime_error(baked_error);
        const float no_environment[]{0, 0, 0};
        const auto object_light =
            xray::render::vulkan::evaluate_object_lighting({0, 0, 0}, no_environment, no_environment, no_environment,
                                                           xray::render::vulkan::VulkanTraceAll, baked_sources);
        object_push.matrices[2] = pack_model_hemi(object_light.hemi_cube[0], object_light.hemi_cube[3]);
        for (float depth : {.1f, .8f})
        {
            object_push.mvp[14] = depth;
            const auto lit_object = execute(object_fragment, object_push, false, object_vertex);
            near(lit_object[1][3], .875f + .125f * .563f,
                 "authored local hemi reaches object at near/far camera depth");
        }
        vkDestroyShaderModule(device, object_vertex, nullptr);
        vkDestroyShaderModule(device, object_fragment, nullptr);
        // GL level wallmarks sample their texture without baked vertex color.
        // A neutral 0.5 border must stay neutral even with black COLOR0 and
        // alpha=1; otherwise multiply-2x draws a dark rectangle around fire.
        const auto before_decal = triangle;
        const auto before_decal_texture = pixels[0];
        fill(memory, input[0], {.5f, .5f, .5f, 1});
        for (auto &v : triangle)
        {
            v.baked[0] = v.baked[1] = v.baked[2] = 0;
            v.baked[3] = 1;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        auto decal_shader = module("wallmark_multiply_2x.ps.spv");
        const auto decal = execute(decal_shader, push);
        for (unsigned c = 0; c < 3; ++c)
            near(decal[0][c], .5f, "level decal neutral border ignores baked COLOR0");
        const auto neutral_floor = execute(decal_shader, push, false, VK_NULL_HANDLE, false, false, false, true);
        for (unsigned c = 0; c < 3; ++c)
            near(neutral_floor[0][c], .2f + float(c) * .1f, "multiply decal border leaves lit floor unchanged");
        fill(memory, input[0], {.2f, .2f, .2f, 1});
        const auto burned_floor = execute(decal_shader, push, false, VK_NULL_HANDLE, false, false, false, true);
        for (unsigned c = 0; c < 3; ++c)
            near(burned_floor[0][c], (.2f + float(c) * .1f) * .4f,
                 "burn mark detail survives without a rectangular border");
        vkDestroyShaderModule(device, decal_shader, nullptr);
        triangle = before_decal;
        fill(memory, input[0], before_decal_texture);
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        auto flat_result = execute(flat, push);
        near(flat_result[0][0], pixels[0][0], "base.R");
        near(flat_result[0][1], pixels[0][1], "static albedo does not multiply packed vertex light twice");
        near(flat_result[0][3], pixels[1][1], "lightmap sun channel G");
        near(flat_result[1][3], pixels[1][3] * .75f, "lightmap hemi channel A");
        push.modes[3] = 2.f;
        push.factors[0] = .5f;
        auto material = execute(flat, push);
        near(material[2][1], 2.5f / 3.f, "material class and weight in surface G");
        push.modes[3] = 1.f;
        push.factors[0] = 0.f;
        push.modes[0] = 2;
        auto bump = execute(flat, push);
        near(bump[2][0], pixels[7][0] * pixels[7][0], "bump gloss red");
        near(bump[2][3], pixels[8][2], "bump auxiliary height blue");
        fill(memory, input[7], {.5f, .5f, .5f, .5f});
        fill(memory, input[8], {.5f, .5f, .5f, .5f});
        const auto neutral_bump = execute(flat, push);
        near(neutral_bump[1][0], .5f, "degenerate bump retains geometric normal X");
        near(neutral_bump[1][2], 1.f, "degenerate bump retains geometric normal Z");
        fill(memory, input[7], pixels[7]);
        fill(memory, input[8], pixels[8]);
        push.modes[2] = 1;
        auto normal = execute(flat, push);
        if (std::abs(normal[1][0] - bump[1][0]) < .02f)
            throw std::runtime_error("external normal slot 15 did not change output normal");
        // Distance/resolution change the magnitude of UV derivatives, but
        // must not change the tangent-space lighting for the same surface.
        for (auto &vertex : triangle)
            for (float &uv : vertex.uv)
                uv *= 0.00001f;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        auto distant_normal = execute(flat, push);
        for (unsigned channel = 0; channel < 3; ++channel)
            near(distant_normal[1][channel], normal[1][channel], "normal mapping at small UV derivatives");
        for (auto &vertex : triangle)
            for (float &uv : vertex.uv)
                uv *= 100000.f;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        // Authored object-space frames take precedence even if UV derivatives
        // reverse or collapse as the mesh changes distance/projected orientation.
        const auto saved_frame_vertices = triangle;
        for (auto &v : triangle)
        {
            v.static_tangent[1] = 127;
            v.static_bitangent[0] = -127;
        }
        auto upload_frame_vertices = [&] {
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
        };
        upload_frame_vertices();
        const auto authored_frame = execute(flat, push);
        for (auto &v : triangle)
            for (float &uv : v.uv)
                uv *= -.00001f;
        upload_frame_vertices();
        const auto changed_derivatives = execute(flat, push);
        for (unsigned axis = 0; axis < 3; ++axis)
            near(changed_derivatives[1][axis], authored_frame[1][axis],
                 "static authored tangent ignores view derivatives");
        triangle = saved_frame_vertices;
        upload_frame_vertices();
        push.modes[2] = 0;
        push.modes[1] = 0;
        push.factors[2] = .18f;
        auto emissive = execute(flat, push);
        near(emissive[2][2], .18f, "material emissive channel");
        push.factors[2] = 0;
        auto detailed = execute(detail, push);
        near(detailed[0][0], pixels[0][0] * pixels[2][0] * 2.f, "detail RGB");
        near(detailed[2][0], pixels[7][0] * pixels[7][0] * pixels[2][3] * 2.f, "detail alpha gloss");
        push.modes[1] = 1;
        auto detail_bump = execute(detail, push);
        near(detail_bump[2][0], pixels[7][0] * pixels[7][0] * pixels[9][0] * 2.f, "detail bump gloss");
        images[10].imageView = input[8].view;
        vkUpdateDescriptorSets(device, 1, &writes[10], 0, nullptr);
        auto changed_detail_aux = execute(detail, push);
        if (std::abs(changed_detail_aux[1][0] - detail_bump[1][0]) < .005f &&
            std::abs(changed_detail_aux[1][1] - detail_bump[1][1]) < .005f)
            throw std::runtime_error("detail bump auxiliary slot 10 did not change normal");
        images[10].imageView = input[10].view;
        vkUpdateDescriptorSets(device, 1, &writes[10], 0, nullptr);
        push.modes[0] = 0;
        push.modes[1] = 0;
        push.factors[3] = 15;
        auto terrain_result = execute(terrain, push);
        const float weight_sum = pixels[2][0] + pixels[2][1] + pixels[2][2] + pixels[2][3];
        float terrain_red = 0, terrain_gloss = 0;
        for (int i = 0; i < 4; ++i)
        {
            const float weight = pixels[2][i] / weight_sum;
            terrain_red += pixels[3 + i][0] * weight;
            terrain_gloss += pixels[11 + i][0] * pixels[11 + i][0] * weight;
        }
        near(terrain_result[0][0], pixels[0][0] * 2.f * terrain_red, "terrain mask R/G/B/A slots");
        near(terrain_result[2][0], terrain_gloss, "terrain four bump gloss slots");
        near(terrain_result[1][3], pixels[0][3] * .75f, "terrain hemi comes from base alpha, not atlas lightmap");
        float terrain_height = 0;
        for (int i = 0; i < 4; ++i)
            terrain_height += pixels[16 + i][2] * pixels[2][i] / weight_sum;
        near(terrain_result[2][3], terrain_height, "terrain four auxiliary height slots");
        // EnvironmentParams has a distinct push layout: factor at 80,
        // surface modes at 96, surface factors at 112.
        Push environment = push;
        environment.modes[0] = .7f;
        environment.modes[1] = .4f;
        environment.modes[2] = .6f;
        environment.factors[0] = 0;
        environment.factors[1] = 0;
        environment.factors[2] = 0;
        environment.factors[3] = 1;
        auto reflected_2d = execute(laemb, environment);
        near(reflected_2d[0][0], pixels[1][0] + pixels[2][0] * .7f, "LaEmB environment 2D red");
        near(reflected_2d[0][1], pixels[1][1] + pixels[2][1] * .4f, "LaEmB environment 2D green");
        images[2].imageView = cube.view;
        vkUpdateDescriptorSets(device, 1, &writes[2], 0, nullptr);
        environment.factors[3] = 1;
        auto reflected_cube = execute(reflection, environment, true);
        near(reflected_cube[0][0], cube_pixel[0] * (1 - pixels[0][3]) + pixels[0][0] * pixels[0][3],
             "reflection cube red");
        near(reflected_cube[0][1], cube_pixel[1] * (1 - pixels[0][3]) + pixels[0][1] * pixels[0][3],
             "reflection cube green");
        images[2].imageView = input[2].view;
        vkUpdateDescriptorSets(device, 1, &writes[2], 0, nullptr);
        push.threshold = 200.f / 255.f;
        auto particle = execute(deferred_particle, push);
        near(particle[0][0], pixels[0][0] * .5f, "deferred particle tint red");
        near(particle[0][1], pixels[0][1] * .25f, "deferred particle tint green");
        near(particle[0][3], pixels[0][3], "deferred particle alpha");
        push.threshold = .9f;
        particle = execute(deferred_particle, push);
        near(particle[0][0], 0.f, "deferred particle cutout at higher alpha reference");
        // Transparent texels may retain bright RGB in particle atlases.
        // The desktop particle shader discards zero alpha before additive blending.
        fill(memory, input[0], {1.f, .5f, 1.f, 0.f});
        auto transparent_add = execute(additive_particle, push);
        near(transparent_add[0][0], 0.f, "zero-alpha additive particle RGB must be discarded");
        auto transparent_world_set = execute(additive_world_set, push);
        near(transparent_world_set[0][0], .5f, "S_SET additive preserves its alpha-independent RGB semantics");
        fill(memory, input[0], pixels[0]);
        // The same RG distortion field must output sampled scene color,
        // irrespective of its retained RGB, instead of emitting yellow quads.
        fill(memory, input[0], {.8f, .7f, .1f, 1.f});
        Push distort = push;
        distort.threshold = 0;
        distort.scale = 1;
        distort.padding[0] = 1;
        auto refracted = execute(distort_ps, distort, false, distort_vs);
        for (unsigned channel = 0; channel < 3; ++channel)
            near(refracted[0][channel], pixels[3][channel], "distortion reads scene color, not vector RGB");
        near(refracted[0][3], .495f, "distortion weight uses atlas alpha and particle RGB");
        // A horizontal scene gradient makes the displacement itself visible
        // in readback, including its direction and the neutral 127/255 field.
        void *gradient{};
        check(vkMapMemory(device, input[3].memory, 0, VK_WHOLE_SIZE, 0, &gradient));
        VkImageSubresource gradient_subresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout gradient_layout{};
        vkGetImageSubresourceLayout(device, input[3].image, &gradient_subresource, &gradient_layout);
        for (unsigned x = 0; x < 128; ++x)
        {
            const float rgba[]{float(x) / 127.f, .3f, .4f, 1.f};
            std::memcpy(static_cast<uint8_t *>(gradient) + gradient_layout.offset + x * sizeof(rgba), rgba,
                        sizeof(rgba));
        }
        vkUnmapMemory(device, input[3].memory);
        fill(memory, input[0], {127.f / 255.f, 127.f / 255.f, .1f, 1.f});
        auto neutral = execute(distort_ps, distort, false, distort_vs);
        near(neutral[0][0], 64.f / 127.f, "neutral distortion leaves the sampled scene coordinate unchanged");
        fill(memory, input[0], {1.f, 127.f / 255.f, .1f, 1.f});
        auto shifted = execute(distort_ps, distort, false, distort_vs);
        near(shifted[0][0], 67.f / 127.f, "distortion offsets the scene sample to the right");
        fill(memory, input[0], {.8f, .7f, .1f, 0.f});
        auto no_distortion = execute(distort_ps, distort, false, distort_vs);
        near(no_distortion[0][0], 0.f, "zero-weight distortion is discarded");
        // Execute the actual UI stages with a deliberately purple/yellow
        // distortion atlas and a differently colored completed-menu snapshot.
        // The old path displayed the atlas itself. The fixed path samples scene.
        for (unsigned i = 0; i < triangle.size(); ++i)
        {
            triangle[i].position[0] = i == 1 ? 256.f : 0.f;
            triangle[i].position[1] = i == 2 ? 2.f : 0.f;
            for (float &tint : triangle[i].baked)
                tint = 1.f;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        Push ui_push{};
        ui_push.mvp[0] = 128.f;
        ui_push.mvp[1] = 1.f;
        ui_push.mvp[3] = -1.f;
        fill(memory, input[0], {.8f, .1f, .9f, .5f});
        fill(memory, input[3], {.12f, .65f, .23f, 1.f}, 1, 128);
        auto menu = execute(ui_ps, ui_push, false, ui_vs);
        near(menu[0][0], .12f, "menu PP red comes from scene, not distortion atlas");
        near(menu[0][1], .65f, "menu PP green comes from scene");
        near(menu[0][2], .23f, "menu PP blue comes from scene");
        near(menu[0][3], 1.f, "menu PP replaces the scene sample");
        ui_push.mvp[3] = 1.f;
        auto ordinary_ui = execute(ui_ps, ui_push, false, ui_vs);
        near(ordinary_ui[0][0], .8f, "ordinary UI still draws its color texture");
        near(ordinary_ui[0][3], .5f, "ordinary UI retains alpha");
        ui_push.mvp[3] = -1.f;
        check(vkMapMemory(device, input[3].memory, 0, VK_WHOLE_SIZE, 0, &gradient));
        for (unsigned x = 0; x < 128; ++x)
        {
            const float c = x % 5 < 2 ? .8f : .05f;
            const float rgba[]{c, c, c, 1};
            std::memcpy(static_cast<uint8_t *>(gradient) + gradient_layout.offset + x * 16, rgba, 16);
        }
        vkUnmapMemory(device, input[3].memory);
        fill(memory, input[0], {1.f, 0.f, .9f, .5f});
        execute(ui_ps, ui_push, false, ui_vs);
        for (unsigned x = 0; x < 128; ++x)
            near(last_ui_pixels[x][0], (x % 5 < 2 ? .8f : .05f) * 1.02f,
                 "menu selector preserves every glyph-pattern coordinate");
        // Selection can change brightness slightly, but never shift labels.
        check(vkMapMemory(device, input[3].memory, 0, VK_WHOLE_SIZE, 0, &gradient));
        for (unsigned x = 0; x < 128; ++x)
        {
            const float rgba[]{float(x) / 127.f, .3f, .4f, 1.f};
            std::memcpy(static_cast<uint8_t *>(gradient) + gradient_layout.offset + x * sizeof(rgba), rgba,
                        sizeof(rgba));
        }
        vkUnmapMemory(device, input[3].memory);
        fill(memory, input[0], {127.f / 255.f, 127.f / 255.f, .9f, 1.f});
        execute(ui_ps, ui_push, false, ui_vs);
        for (unsigned x = 0; x < 128; ++x)
            near(last_ui_pixels[x][0], float(x) / 127.f * 1.04f, "neutral menu field preserves all scene coordinates");
        fill(memory, input[0], {1.f, 127.f / 255.f, .9f, .5f});
        execute(ui_ps, ui_push, false, ui_vs);
        for (unsigned x = 0; x < 128; ++x)
            near(last_ui_pixels[x][0], float(x) / 127.f * 1.02f, "menu selection never displaces label pixels");
        if (argc >= 3)
        {
            std::ofstream preview(argv[2], std::ios::binary);
            preview << "P6\n128 96\n255\n";
            for (unsigned y = 0; y < 96; ++y)
                for (unsigned x = 0; x < 128; ++x)
                {
                    const std::array<float, 3> rgb =
                        y < 32 ? std::array<float, 3>{float(x) / 127.f, .3f, .4f}
                        : y < 64
                            ? std::array<float, 3>{1.f, 127.f / 255.f, .9f}
                            : std::array<float, 3>{last_ui_pixels[x][0], last_ui_pixels[x][1], last_ui_pixels[x][2]};
                    for (float value : rgb)
                        preview.put(static_cast<char>(std::clamp(value, 0.f, 1.f) * 255.f + .5f));
                }
            if (!preview)
                throw std::runtime_error("cannot save menu GPU preview");
        }
        fill(memory, input[0], {.8f, .1f, .9f, 0.f});
        auto transparent_ui = execute(ui_ps, ui_push, false, ui_vs);
        near(transparent_ui[0][3], 0.f, "transparent PP atlas texels leave scene untouched");
        // World UI/tracer path: clip-space depth must survive the UI API,
        // even when the attachment was cleared for HUD rendering.
        auto saved_triangle = triangle;
        for (unsigned i = 0; i < 3; ++i)
        {
            triangle[i].position[0] = i == 1 ? 3.f : -1.f;
            triangle[i].position[1] = i == 2 ? -3.f : 1.f;
            triangle[i].clip_depth_w[0] = .6f;
            triangle[i].clip_depth_w[1] = 1;
            triangle[i].uv[0] = i == 1 ? 2.f : 0.f;
            triangle[i].uv[1] = .5f;
        }
        auto write_triangle = [&]() {
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
        };
        write_triangle();
        fill(memory, input[0], {.8f, .1f, .9f, .5f});
        fill(memory, input[3], {.4f, .4f, .4f, 1});
        ui_push.mvp[3] = 21; // World depth + alpha-add mode 5.
        execute(ui_ps, ui_push, false, ui_vs);
        for (const auto &pixel : last_ui_pixels)
            near(pixel[0], 0.f, "world tracer behind a wall is occluded");
        for (auto &v : triangle)
            v.clip_depth_w[0] = .2f;
        write_triangle();
        execute(ui_ps, ui_push, false, ui_vs);
        for (const auto &pixel : last_ui_pixels)
            near(pixel[0], .8f, "world tracer in front of a wall survives");
        // A late forward occluder can be nearer than the depth snapshot.
        // The world UI pipeline also tests the live attachment before HUD clear.
        auto late_occlusion = execute(ui_ps, ui_push, false, ui_vs, false, false, true);
        near(late_occlusion[0][0], 0.f, "late world depth also occludes the tracer");
        // Perspective UV differs from affine interpolation when clip W varies.
        triangle[1].position[0] *= 4;
        triangle[1].position[1] *= 4;
        triangle[1].clip_depth_w[0] *= 4;
        triangle[1].clip_depth_w[1] = 4;
        write_triangle();
        check(vkMapMemory(device, input[0].memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        VkSubresourceLayout tracer_layout{};
        VkImageSubresource tracer_sub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        vkGetImageSubresourceLayout(device, input[0].image, &tracer_sub, &tracer_layout);
        for (unsigned y = 0; y < 2; ++y)
            for (unsigned x = 0; x < 2; ++x)
            {
                const float color[]{x ? .8f : .2f, .1f, .1f, 1};
                std::memcpy(static_cast<uint8_t *>(mapped) + tracer_layout.offset + y * tracer_layout.rowPitch + x * 16,
                            color, 16);
            }
        vkUnmapMemory(device, input[0].memory);
        execute(ui_ps, ui_push, false, ui_vs);
        near(last_ui_pixels[100][0], .2f, "world tracer UV uses perspective interpolation");
        // A triangle crossing the near plane must be clipped, not flattened
        // into an overlay or dropped as a whole.
        for (unsigned i = 0; i < 3; ++i)
        {
            triangle[i].position[0] = i == 1 ? 3.f : -1.f;
            triangle[i].position[1] = i == 2 ? -3.f : 1.f;
            triangle[i].clip_depth_w[0] = i == 0 ? -.5f : .5f;
            triangle[i].clip_depth_w[1] = 1;
        }
        write_triangle();
        fill(memory, input[3], {.9f, .9f, .9f, 1});
        execute(ui_ps, ui_push, false, ui_vs);
        unsigned visible_pixels = 0;
        for (const auto &pixel : last_ui_pixels)
            if (pixel[0] > .01f)
                ++visible_pixels;
        if (!visible_pixels || visible_pixels >= 128)
            throw std::runtime_error("world tracer near-plane clipping lost homogeneous depth");
        triangle = saved_triangle;
        write_triangle();
        std::array<Vertex, 6> sprite_vertices{};
        const unsigned order[]{0, 1, 2, 0, 2, 3};
        for (unsigned i = 0; i < 6; ++i)
        {
            const auto c = particle_sprite_corner(order[i], 1.f, 1.f, 0.f);
            auto &v = sprite_vertices[i];
            v.position[0] = c.right;
            v.position[1] = c.up;
            v.normal[2] = 1;
            v.uv[0] = c.u;
            v.uv[1] = c.v;
            for (float &color : v.baked)
                color = 1;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, sprite_vertices.data(), sizeof(sprite_vertices));
        vkUnmapMemory(device, vertex_memory);
        check(vkMapMemory(device, input[0].memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        VkImageSubresource sprite_sub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout sprite_layout{};
        vkGetImageSubresourceLayout(device, input[0].image, &sprite_sub, &sprite_layout);
        for (unsigned y = 0; y < 2; ++y)
            for (unsigned x = 0; x < 2; ++x)
            {
                const float rgba[]{y == 0 ? .9f : .1f, 0.f, y == 0 ? .1f : .9f, 1.f};
                std::memcpy(static_cast<uint8_t *>(mapped) + sprite_layout.offset + y * sprite_layout.rowPitch + x * 16,
                            rgba, 16);
            }
        vkUnmapMemory(device, input[0].memory);
        execute(additive_particle, push, false, VK_NULL_HANDLE, true);
        for (unsigned x = 0; x < 2; ++x)
        {
            near(sprite_pixels[x][0], .9f, "upright particle: atlas top is camera-up");
            near(sprite_pixels[x + 2][2], .9f, "upright particle: atlas bottom is camera-down");
        }
        for (float pitch : {.4f, 1.2f})
        {
            const auto basis = particle_path_basis({0, 1, 0}, {0, -std::sin(pitch), std::cos(pitch)});
            for (unsigned i = 0; i < 6; ++i)
            {
                const auto c = particle_sprite_corner(order[i], 1, 1, 0);
                for (unsigned axis = 0; axis < 3; ++axis)
                    sprite_vertices[i].position[axis] = basis.right[axis] * c.right + basis.up[axis] * c.up;
            }
            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, sprite_vertices.data(), sizeof(sprite_vertices));
            vkUnmapMemory(device, vertex_memory);
            // Normalize the projected height to the two-pixel readback target.
            auto tilted_push = push;
            tilted_push.mvp[9] = std::tan(pitch);
            execute(additive_particle, tilted_push, false, VK_NULL_HANDLE, true);
            for (unsigned x = 0; x < 2; ++x)
            {
                near(sprite_pixels[x][0], .9f, "path-aligned flame keeps upper atlas at camera pitch");
                near(sprite_pixels[x + 2][2], .9f, "path-aligned flame keeps lower atlas at camera pitch");
            }
        }
        // Run the actual weather combine. Rapidly varying normals must not
        // select dark lower cube mips: the original hmodel explicitly uses LOD 0.
        sampler_info.maxLod = 6; // The original sampler used maxLod=0; use an unrestricted sampler below.
        VkSampler environment_sampler{};
        check(vkCreateSampler(device, &sampler_info, nullptr, &environment_sampler));
        for (unsigned binding : {0u, 1u, 4u, 5u})
        {
            VkDescriptorImageInfo info{environment_sampler, weather_cube.view, VK_IMAGE_LAYOUT_GENERAL};
            VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            write.dstSet = weather_environment_set;
            write.dstBinding = binding;
            write.descriptorCount = 1;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &info;
            vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        }
        fill(memory, input[0], {.25f, .4f, .6f, 1.f});
        fill(memory, input[2], {0.f, 1.f / 3.f, 0.f, 0.f});
        fill(memory, input[19], {.5f, 0.f, 0.f, 0.f});
        Push weather_push{};
        float *w = reinterpret_cast<float *>(&weather_push);
        w[2] = 1000.f;
        w[13] = -1.f;
        w[20] = w[21] = w[22] = 1.f;
        // packHalf2x16(0,100) and (1000,1000).
        const uint32_t near_bits = 0x56400000u, far_bits = 0x63d063d0u;
        std::memcpy(w + 7, &near_bits, 4);
        std::memcpy(w + 11, &far_bits, 4);
        execute(weather_ps, weather_push, false, weather_vs);
        if (argc == 4)
        {
            std::ofstream preview(argv[3], std::ios::binary);
            preview << "P6\n128 32\n255\n";
            for (unsigned y = 0; y < 32; ++y)
                for (const auto &pixel : last_ui_pixels)
                    for (unsigned c = 0; c < 3; ++c)
                        preview.put(static_cast<char>(std::clamp(pixel[c], 0.f, 1.f) * 255.f + .5f));
            if (!preview)
                throw std::runtime_error("cannot save weather GPU readback");
        }
        const std::array<float, 3> expected_lighting{.25f * (.04f + 1.6f * 1.6f), .4f * (.04f + 1.08f * 1.08f),
                                                     .6f * (.04f + .64f * .64f)};
        for (unsigned x = 0; x < 128; ++x)
            for (unsigned channel = 0; channel < 3; ++channel)
                near(last_ui_pixels[x][channel], expected_lighting[channel],
                     "weather diffuse reads cube mip zero across rapidly changing normals");
        w[16] = w[17] = w[18] = 1.f;
        execute(weather_ps, weather_push, false, weather_vs);
        for (unsigned x = 0; x < 128; ++x)
        {
            const float visibility = x == 0 ? .2f : x == 127 ? .8f : x % 2 ? .6f : .4f;
            for (unsigned c = 0; c < 3; ++c)
                near(last_ui_pixels[x][c], expected_lighting[c] + pixels[0][c] * .6f * visibility,
                     "shadow checker comparisons use continuous weighted PCF");
        }
        // Execute the real local-light shader and additive blend. The lamp
        // must illuminate a receiver, follow its cone/cookie and respect depth.
        fill(memory, input[0], {.4f, .3f, .2f, 1});
        fill(memory, input[1], {.5f, .5f, 0, 1});
        fill(memory, input[2], {.5f, 0, 0, 0});
        fill(memory, input[5], {0, 0, 0, 0});
        fill(memory, input[18], {1, 1, 1, 1});
        for (unsigned i = 0; i < 16; i += 5)
            local_uniform.inverse_view_projection[i] = 1;
        local_uniform.position_range[3] = 10;
        local_uniform.direction_cone[2] = 1;
        local_uniform.direction_cone[3] = std::cos(.5f);
        local_uniform.spot_right[0] = 1;
        local_uniform.spot_right[3] = std::tan(.5f);
        local_uniform.color_type[0] = local_uniform.color_type[1] = local_uniform.color_type[2] =
            local_uniform.color_type[3] = 1;
        write_local();
        auto lamp = execute(local_fragment, push, false, weather_vs);
        const float lamp_value = .1f + .4f * (1.f - .05f * .05f);
        near(lamp[0][0], lamp_value, "torch illuminates world through additive local pass");
        local_uniform.direction_cone[2] = -1;
        write_local();
        near(execute(local_fragment, push, false, weather_vs)[0][0], .1f, "torch cone excludes opposite direction");
        local_uniform.direction_cone[2] = 1;
        local_uniform.inverse_view_projection[12] = 5;
        local_uniform.position_range[0] = 5;
        write_local();
        near(execute(local_fragment, push, false, weather_vs)[0][0], lamp_value,
             "torch world reconstruction is translation invariant");
        local_uniform.shadow_params[3] = 1;
        write_local();
        fill(memory, input[18], {0, 0, 0, 1});
        near(execute(local_fragment, push, false, weather_vs)[0][0], .1f, "torch projection rejects dark cookie");
        fill(memory, input[18], {1, 1, 1, 1});
        near(execute(local_fragment, push, false, weather_vs)[0][0], lamp_value,
             "torch projection preserves white cookie");
        local_uniform.inverse_view_projection[12] = local_uniform.position_range[0] = 0;
        for (unsigned i = 0; i < 16; i += 5)
            local_uniform.shadow_matrices[0][i] = 1;
        local_uniform.shadow_params[2] = 1;
        write_local();
        fill(memory, cube, {.25f, 0, 0, 1}, 6);
        near(execute(local_fragment, push, false, weather_vs)[0][0], .1f, "local shadow blocks receiver");
        // A fixed eye sees the same receiver at the center and at the
        // clipped edge of the viewport. The old 5% scissor threshold
        // illuminated the shadowed receiver after turning the camera.
        for (unsigned coverage : {8u, 3u})
        {
            LocalShadowBudget slots{2, 0, 1};
            const auto slot = slots.reserve(true, true, 4.f, 10.f);
            local_uniform.shadow_params[2] = coverage >= 5;
            write_local();
            near(execute(local_fragment, push, false, weather_vs)[0][0],
                 coverage >= 5 ? .1f : lamp_value, "old camera coverage reproduces light popping");
            local_uniform.shadow_params[2] = slot >= 0;
            write_local();
            near(execute(local_fragment, push, false, weather_vs)[0][0], .1f,
                 "fixed camera position retains the lamp shadow on a turn");
        }
        fill(memory, cube, {.75f, 0, 0, 1}, 6);
        near(execute(local_fragment, push, false, weather_vs)[0][0], lamp_value, "local shadow keeps visible receiver");
        // At 50 m, a one-metre caster/receiver gap is only 0.00004 in
        // perspective depth. A fixed 0.002 bias incorrectly removes the shadow.
        local_uniform.inverse_view_projection[10] = 100;
        local_uniform.position_range[3] = 100;
        std::fill_n(local_uniform.shadow_matrices[0], 16, 0.f);
        auto *projection = local_uniform.shadow_matrices[0];
        projection[0] = projection[5] = 1;
        projection[10] = 100.f / 99.9f;
        projection[11] = 1;
        projection[14] = -.1f * 100.f / 99.9f;
        local_uniform.shadow_params[1] = LocalShadowReceiverOffset;
        write_local();
        const auto projected_depth = [&](float z) { return (projection[10] * z + projection[14]) / z; };
        fill(memory, cube, {projected_depth(49), 0, 0, 1}, 6);
        near(execute(local_fragment, push, false, weather_vs)[0][0], .1f,
             "world-space bias preserves distant character shadows");
        fill(memory, cube, {projected_depth(51), 0, 0, 1}, 6);
        near(execute(local_fragment, push, false, weather_vs)[0][0], .4f,
             "world-space bias preserves visible distant receivers");
        // Execute the paired-view foliage shaders, including both endpoint
        // weights and the facet bisector, with the real static vertex layout.
        triangle = prepared_triangle;
        for (auto &v : triangle)
        {
            std::copy_n(v.position, 3, v.normal);
            for (unsigned k=0;k<4;++k) v.static_tangent[k]=v.static_bitangent[k]=0;
            v.static_tangent[2]=v.static_bitangent[2]=127;
            v.baked[0]=v.baked[1]=.5f;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        fill(memory, input[0], {.4f,.3f,.2f,1});
        Push lod_push{};
        for (unsigned i=0;i<16;i+=5) lod_push.mvp[i]=1;
        lod_push.threshold=-96.f/255.f;
        for (float weight : {0.f,.5f,1.f})
        {
            lod_push.scale=weight;
            auto lod_pixel=execute(lod_fragment,lod_push,false,lod_vertex);
            near(lod_pixel[0][0],.4f,"paired foliage preserves albedo");
            near(lod_pixel[1][2],1.f,"paired foliage normal survives view interpolation");
            near(lod_pixel[1][3],.9375f,"paired foliage preserves baked hemisphere");
        }
        fill(memory, input[1], {.5f, 1.f, .5f, .2f});
        lod_push.threshold = 96.f / 255.f;
        const auto atlas_pixel = execute(lod_fragment, lod_push, false, lod_vertex);
        near(atlas_pixel[1][1], 1.f, "foliage uses the authored normal atlas");
        near(atlas_pixel[1][3], .875f + .125f * .5f * .2f * 3.1f, "atlas modulates baked hemisphere");
        lod_push.threshold = 1.1f;
        near(execute(lod_fragment, lod_push, false, lod_vertex)[0][0], 0.f, "LOD fade removes coverage");
        fill(memory, input[1], pixels[1]);
        std::array<Vertex, 6> grass_quad{};
        const float quad_positions[6][2]{{-1, -1}, {1, -1}, {-1, 1}, {-1, 1}, {1, -1}, {1, 1}};
        for (unsigned i = 0; i < 6; ++i)
        {
            auto &v = grass_quad[i];
            v.position[0] = quad_positions[i][0];
            v.position[1] = quad_positions[i][1];
            v.normal[2] = 1;
            v.uv[0] = v.uv[1] = .5f;
            v.lightmap_uv[0] = v.lightmap_uv[1] = .5f;
            v.baked[3] = 1;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, grass_quad.data(), sizeof(grass_quad));
        vkUnmapMemory(device, vertex_memory);
        Push grass_push{};
        for (unsigned i = 0; i < 16; i += 5)
            grass_push.mvp[i] = 1;
        fill(memory, input[0], {.7f, .3f, .2f, 1});
        fill(memory, input[1], {.2f, .2f, .2f, .5f});
        const auto grass_result = execute(flat, grass_push, false, instanced_vertex, true);
        for (const auto &pixel : sprite_pixels)
            near(pixel[0], .7f, "both grass instances retain prototype albedo");
        near(grass_result[1][2], 1, "first grass normal uses instance yaw");
        check(vkMapMemory(device, readback_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        float right_normal[4];
        std::memcpy(right_normal, static_cast<char *>(mapped) + (128 * 96) * 16 + 16, 16);
        vkUnmapMemory(device, readback_memory);
        near(right_normal[2], 0, "second grass normal rotates independently");
        fill(memory, input[0], {.7f, .3f, .2f, 0});
        grass_push.threshold = .5f;
        const auto grass_cutout = module("level_cutout.ps.spv");
        execute(grass_cutout, grass_push, false, instanced_vertex, true);
        for (const auto &pixel : sprite_pixels)
            near(pixel[0], 0, "instanced cutout discards transparent grass texels");
        vkDestroyShaderModule(device, grass_cutout, nullptr);
        // A real two-dimensional scene exposes vertical sampling errors and
        // stale opaque snapshots that a 128x1 strip cannot detect. The red
        // channel is X, green is Y, and blue marks transparent water.
        const auto scene_image = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 1, 128, 1, 96);
        check(vkMapMemory(device, scene_image.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        VkImageSubresource scene_subresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout scene_layout{};
        vkGetImageSubresourceLayout(device, scene_image.image, &scene_subresource, &scene_layout);
        for (unsigned y = 0; y < 96; ++y)
            for (unsigned x = 0; x < 128; ++x)
            {
                const float rgba[]{float(x) / 127, float(y) / 95, .85f, 1};
                std::memcpy(static_cast<uint8_t *>(mapped) + scene_layout.offset + y * scene_layout.rowPitch + x * 16,
                            rgba, sizeof(rgba));
            }
        vkUnmapMemory(device, scene_image.memory);
        check(vkResetCommandPool(device, commands, 0));
        VkCommandBufferBeginInfo scene_begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        check(vkBeginCommandBuffer(cmd, &scene_begin));
        VkImageMemoryBarrier scene_barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        scene_barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
        scene_barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        scene_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        scene_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        scene_barrier.srcQueueFamilyIndex = scene_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        scene_barrier.image = scene_image.image;
        scene_barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &scene_barrier);
        check(vkEndCommandBuffer(cmd));
        VkSubmitInfo scene_submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        scene_submit.commandBufferCount = 1;
        scene_submit.pCommandBuffers = &cmd;
        check(vkQueueSubmit(queue, 1, &scene_submit, VK_NULL_HANDLE));
        check(vkQueueWaitIdle(queue));
        scene_test = true;
        triangle = prepared_triangle;
        for (auto &vertex : triangle)
            vertex.position[2] = .5f;
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        fill(memory, input[19], {1, 0, 0, 1});
        fill(memory, input[0], {127.f / 255, 127.f / 255, .1f, 1});
        Push scene_push = push;
        scene_push.threshold = 0;
        scene_push.scale = 1.f / 128;
        scene_push.padding[0] = 1.f / 96;
        scene_images[0].imageView = scene_image.view;
        vkUpdateDescriptorSets(device, 1, &scene_writes[0], 0, nullptr);
        execute(distort_ps, scene_push, false, distort_vs);
        for (unsigned y = 1; y < 95; y += 7)
            for (unsigned x = 1; x < 127; x += 9)
            {
                const auto &p = scene_pixels[y * 128 + x];
                near(p[0], float(x) / 127, "2D neutral refraction preserves X");
                near(p[1], float(y) / 95, "2D neutral refraction preserves Y");
                near(p[2], .85f, "2D refraction retains transparent water");
            }
        // Both axes, both signs, and all four screen edges: a one-row
        // fixture cannot expose a vertical inversion or a wrong Y pixel size.
        for (const auto &field : std::array<std::array<float, 4>, 4>{{
                 {1, 127.f / 255, 0, 1}, {0, 127.f / 255, 0, 1},
                 {127.f / 255, 1, 0, 1}, {127.f / 255, 0, 0, 1}}})
        {
            fill(memory, input[0], field);
            execute(distort_ps, scene_push, false, distort_vs);
            for (unsigned y : {0u, 32u, 63u, 95u})
                for (unsigned x : {0u, 42u, 85u, 127u})
                {
                    const float sx = std::clamp((x + .5f) / 128 + (field[0] - 127.f / 255) * .05f,
                                                .5f / 128, 1 - .5f / 128);
                    const float sy = std::clamp((y + .5f) / 96 + (field[1] - 127.f / 255) * .05f,
                                                .5f / 96, 1 - .5f / 96);
                    const auto &p = scene_pixels[y * 128 + x];
                    near(p[0], (sx * 128 - .5f) / 127, "2D refraction X displacement and edge clamp");
                    near(p[1], (sy * 96 - .5f) / 95, "2D refraction Y displacement and edge clamp");
                }
        }
        // Foreground geometry must not be pulled into the particle.
        fill(memory, input[19], {0, 0, 0, 1});
        execute(distort_ps, scene_push, false, distort_vs);
        near(scene_pixels[32 * 128 + 42][1], 32.f / 95, "2D foreground depth cancels displaced sampling");
        fill(memory, input[19], {1, 0, 0, 1});
        fill(memory, input[0], {127.f / 255, 127.f / 255, .1f, 1});
        // Encode the particle field first, then use that output to displace
        // the complete scene. No scene color enters the particle pass.
        fill(memory, input[0], {1, 127.f / 255, 0, 1});
        scene_push.padding[1] = 1;
        execute(distort_ps, scene_push, false, distort_vs);
        near(scene_pixels[48 * 128 + 64][0], 1.f, "particle pass stores field rather than scene color");
        const auto field = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                 1, 128, 1, 96);
        VkSubresourceLayout field_layout{};
        vkGetImageSubresourceLayout(device, field.image, &scene_subresource, &field_layout);
        check(vkMapMemory(device, field.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        for (unsigned y = 0; y < 96; ++y)
            std::memcpy(static_cast<uint8_t *>(mapped) + field_layout.offset + y * field_layout.rowPitch,
                        scene_pixels.data() + y * 128, 128 * 16);
        vkUnmapMemory(device, field.memory);
        check(vkResetCommandPool(device, commands, 0));
        check(vkBeginCommandBuffer(cmd, &scene_begin));
        scene_barrier.image = field.image;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &scene_barrier);
        check(vkEndCommandBuffer(cmd));
        check(vkQueueSubmit(queue, 1, &scene_submit, VK_NULL_HANDLE));
        check(vkQueueWaitIdle(queue));
        float scene_constants[36]{};
        for (unsigned i = 0; i < 16; i += 5) scene_constants[16+i] = 1;
        check(vkMapMemory(device, weather_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, scene_constants, sizeof(scene_constants));
        vkUnmapMemory(device, weather_memory);
        const VkDescriptorBufferInfo scene_info{weather_uniform, 0, sizeof(scene_constants)};
        const VkDescriptorImageInfo composite_images[]{{sampler, scene_image.view, VK_IMAGE_LAYOUT_GENERAL},
            {sampler, field.view, VK_IMAGE_LAYOUT_GENERAL}, {sampler, input[19].view, VK_IMAGE_LAYOUT_GENERAL}};
        VkWriteDescriptorSet composite_writes[4]{};
        for (uint32_t i = 0; i < 4; ++i)
        {
            auto &write = composite_writes[i];
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = composite_set;
            write.dstBinding = i;
            write.descriptorCount = 1;
            write.descriptorType = i < 3 ? VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            if (i < 3) write.pImageInfo = &composite_images[i];
            else write.pBufferInfo = &scene_info;
        }
        vkUpdateDescriptorSets(device, 4, composite_writes, 0, nullptr);
        execute(composite_ps, scene_push, false, weather_vs);
        const auto &composited = scene_pixels[48 * 128 + 64];
        near(composited[0], (64.f + (1.f - 127.f / 255) * .05f * 128) / 127, "field displaces complete scene once");
        near(composited[1], 48.f / 95, "neutral vertical component");
        near(composited[2], .85f, "composite preserves water in scene snapshot");
        // A ray that exits the screen above a flat water surface must not
        // import a landmark from the vertically mirrored opaque frame.
        for (unsigned i = 0; i < 16; i += 5)
            scene_constants[i] = 1.f;
        scene_constants[33] = 1.5f;
        check(vkMapMemory(device, weather_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, scene_constants, sizeof(scene_constants));
        vkUnmapMemory(device, weather_memory);
        for (auto& vertex : triangle)
        {
            vertex.normal[0] = vertex.normal[2] = 0.f;
            vertex.normal[1] = 1.f;
        }
        check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle));
        vkUnmapMemory(device, vertex_memory);
        fill(memory, input[0], {.5f, .5f, .5f, 1});
        scene_push.padding[1] = .5f;
        for (float time : {0.f, .12f, .24f})
        {
            scene_push.threshold = time;
            execute(water_ps, scene_push, false, water_vs);
            near(scene_pixels[80 * 128 + 64][1], .82f * (80.f / 95.f) + .18f * .16f,
                 "moving water ray miss samples only the submerged scene");
        }
        scene_push.threshold = 0.f;
        {
            using namespace motion_scene;
            constexpr auto HostImage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            const auto atlas = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, HostImage,
                                     1, Width, 1, Height);
            const auto wall_depth = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT, HostImage,
                                          1, Width, 1, Height);
            auto upload_pixels = [&](const Image& target, const Frame& pixels,
                                     VkImageLayout old_layout = VK_IMAGE_LAYOUT_GENERAL) {
                VkSubresourceLayout layout{};
                vkGetImageSubresourceLayout(device, target.image, &scene_subresource, &layout);
                check(vkMapMemory(device, target.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
                for (unsigned y = 0; y < Height; ++y)
                    std::memcpy(static_cast<uint8_t*>(mapped) + layout.offset + y * layout.rowPitch,
                                pixels.data() + y * Width, Width * sizeof(Pixel));
                vkUnmapMemory(device, target.memory);
                check(vkResetCommandPool(device, commands, 0));
                check(vkBeginCommandBuffer(cmd, &scene_begin));
                scene_barrier.image = target.image;
                scene_barrier.oldLayout = old_layout;
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     0, 0, nullptr, 0, nullptr, 1, &scene_barrier);
                check(vkEndCommandBuffer(cmd));
                check(vkQueueSubmit(queue, 1, &scene_submit, VK_NULL_HANDLE));
                check(vkQueueWaitIdle(queue));
            };
            const auto update_texture = [&](VkWriteDescriptorSet write, const Image& target) {
                const VkDescriptorImageInfo info{sampler, target.view, VK_IMAGE_LAYOUT_GENERAL};
                write.pImageInfo = &info;
                vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
            };
            // Set XRAY_VK_MOTION_FRAMES to an existing directory to export
            // the paired 48-frame reproductions along with the pixel checks.
            const char* output = std::getenv("XRAY_VK_MOTION_FRAMES");
            const unsigned frame_count = output ? 48 : 3;
            auto save_frame = [&](const char* scene, unsigned number, const Frame& left, const Frame& right) {
                if (output)
                    write_pair(std::string(output) + "/" + scene + "-" + std::to_string(100 + number) + ".ppm",
                               left, right);
            };
            const auto lake_camera = Camera({0, 1.55f, 0}, 0, .21f, .48f);
            const auto lake_projection = lake_camera.projection();
            const auto lake_inverse = lake_camera.inverse_projection();
            std::copy(lake_projection.begin(), lake_projection.end(), scene_constants);
            std::copy(lake_inverse.begin(), lake_inverse.end(), scene_constants + 16);
            std::copy(lake_camera.eye.begin(), lake_camera.eye.end(), scene_constants + 32);
            check(vkMapMemory(device, weather_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, scene_constants, sizeof(scene_constants));
            vkUnmapMemory(device, weather_memory);

            Frame lake{}, ring{};
            std::array<bool, Width * Height> above_water{};
            for (unsigned y = 0; y < Height; ++y)
                for (unsigned x = 0; x < Width; ++x)
                {
                    const auto ray = lake_camera.ray(x, y);
                    const auto index = y * Width + x;
                    above_water[index] = ray[1] >= 0.f || -lake_camera.eye[1] * ray[2] / ray[1] > 11.f;
                    if (above_water[index])
                    {
                        const float post_x = 11.f * ray[0] / ray[2];
                        const bool post = std::abs(post_x - .28f * std::round(post_x / .28f)) < .04f;
                        lake[index] = post ? Pixel{.24f, .19f, .11f, 1.f} : Pixel{.61f, .72f, .74f, 1.f};
                    }
                    else
                    {
                        const float t = -lake_camera.eye[1] / ray[1];
                        const float pattern = .05f * std::sin(t * ray[0] * 15.f + t * ray[2] * 7.f);
                        lake[index] = {.20f + pattern, .36f + pattern, .42f + pattern, 1.f};
                    }
                    const float u = (2.f * (x + .5f) / Width - 1.f);
                    const float v = (2.f * (y + .5f) / Height - 1.f);
                    const float r = std::hypot(u, v);
                    const float weight = std::max(0.f, 1.f - std::abs(r - .72f) * 15.f);
                    ring[index] = {127.f / 255.f + .44f * u, 127.f / 255.f + .44f * v, 0.f, weight};
                }
            upload_pixels(scene_image, lake);
            upload_pixels(atlas, ring, VK_IMAGE_LAYOUT_PREINITIALIZED);
            update_texture(writes[0], atlas);
            Push ripple = scene_push;
            std::copy(lake_projection.begin(), lake_projection.end(), ripple.mvp);
            ripple.threshold = 0;
            ripple.padding[1] = 1;
            unsigned upright_above = 0, flat_above = 0, flat_below = 0;
            for (unsigned frame = 0; frame < frame_count; ++frame)
            {
                const float progress = float(frame) / (frame_count - 1);
                const unsigned step = std::min(2u, unsigned(progress * 3));
                const float age = progress * 3.f - step;
                const Point contact{(float(step) - 1.f) * .58f, 0.f, 8.8f - step * .75f};
                const Point simulated{contact[0], .25f + age * .35f, contact[2]};
                const Point normal{0.f, 1.f, 0.f};
                const bool water_effect = is_water_contact_effect(step == 1 ?
                    "hit_fx\\effects\\hit_water_hit_distort" : "hit_fx\\effects\\hit_water_hit_distort_big");
                const float radius = .82f + .55f * age;
                Frame upright{}, upright_difference{};
                for (bool horizontal : {false, true})
                {
                    const bool anchored = horizontal && water_effect;
                    const auto impact = anchored ? project_particle_to_water(simulated, contact, normal) : simulated;
                    const auto axes = anchored ? particle_face_basis(normal) :
                        ParticleSpriteBasis{{1, 0, 0}, {0, 1, 0}};
                    std::array<Vertex, 6> quad{};
                    constexpr unsigned corners[]{0, 1, 2, 0, 2, 3};
                    for (unsigned i = 0; i < quad.size(); ++i)
                    {
                        const auto corner = particle_sprite_corner(corners[i], radius, radius, 0.f);
                        auto& vertex = quad[i];
                        for (unsigned axis = 0; axis < 3; ++axis)
                        {
                            vertex.position[axis] = impact[axis] + axes.right[axis] * corner.right +
                                                    axes.up[axis] * corner.up;
                            vertex.baked[axis] = 1.f - .35f * age;
                        }
                        vertex.baked[3] = 1.f;
                        vertex.uv[0] = corner.u;
                        vertex.uv[1] = corner.v;
                    }
                    check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
                    std::memcpy(mapped, quad.data(), sizeof(quad));
                    vkUnmapMemory(device, vertex_memory);
                    distortion_mask = true;
                    execute(distort_ps, ripple, false, distort_vs, true);
                    distortion_mask = false;
                    upload_pixels(field, scene_pixels);
                    execute(composite_ps, ripple, false, weather_vs);
                    Frame changed{};
                    for (unsigned i = 0; i < scene_pixels.size(); ++i)
                    {
                        if (output)
                            changed[i] = above_water[i] ? Pixel{.15f, .15f, .16f, 1.f} : Pixel{.07f, .11f, .14f, 1.f};
                        const float difference = std::abs(scene_pixels[i][0] - lake[i][0]) +
                                                 std::abs(scene_pixels[i][1] - lake[i][1]);
                        if (difference < .045f)
                            continue;
                        if (above_water[i])
                            ++(horizontal ? flat_above : upright_above);
                        else if (horizontal)
                            ++flat_below;
                        if (output)
                            changed[i] = above_water[i] ? Pixel{1.f, .57f, .10f, 1.f} : Pixel{.22f, .81f, 1.f, 1.f};
                    }
                    if (!horizontal)
                    {
                        upright = scene_pixels;
                        upright_difference = changed;
                    }
                    else
                    {
                        save_frame("water", frame, upright, scene_pixels);
                        save_frame("water-diff", frame, upright_difference, changed);
                    }
                }
            }
            if (upright_above < frame_count * 5 || flat_above != 0 || flat_below < frame_count)
                throw std::runtime_error("moving particle scene did not isolate above-water distortion");
            std::cout << "Water, changed pixels over " << frame_count << " frames: upright above water "
                      << upright_above << ", flat above water " << flat_above
                      << ", flat below water " << flat_below << '\n';

            update_texture(writes[0], scene_image);
            update_texture(writes[2], wall_depth);
            fill(memory, input[1], {.5f, .5f, 0, 1});
            fill(memory, input[5], {0, 0, 0, 0});
            fill(memory, cube, {.25f, 0, 0, 1}, 6);
            local_uniform = {};
            constexpr Point Lamp{-.75f, 0.f, 7.75f};
            constexpr float Range = 1.65f;
            std::copy(Lamp.begin(), Lamp.end(), local_uniform.position_range);
            local_uniform.position_range[3] = Range;
            local_uniform.direction_cone[0] = .9486833f;
            local_uniform.direction_cone[2] = .3162278f;
            local_uniform.direction_cone[3] = std::cos(.65f);
            local_uniform.color_type[0] = 2.6f;
            local_uniform.color_type[1] = 2.0f;
            local_uniform.color_type[2] = 1.2f;
            local_uniform.color_type[3] = 1.f;
            local_uniform.shadow_matrices[0][0] = .65f;
            local_uniform.shadow_matrices[0][5] = .65f;
            local_uniform.shadow_matrices[0][14] = .5f;
            local_uniform.shadow_matrices[0][15] = 1.f;
            float legacy_min = 1.f, legacy_max = 0.f, fixed_min = 1.f, fixed_max = 0.f;
            for (unsigned frame = 0; frame < frame_count; ++frame)
            {
                const float phase = float(frame) / (frame_count - 1);
                const float yaw = .8f * std::sin(phase * 3.14159265f);
                const Camera camera({0, 0, 0}, yaw, 0, 1.2f);
                const auto inverse = camera.inverse_projection();
                std::copy(inverse.begin(), inverse.end(), local_uniform.inverse_view_projection);
                Frame wall{}, depth_pixels{};
                for (unsigned y = 0; y < Height; ++y)
                    for (unsigned x = 0; x < Width; ++x)
                    {
                        const auto ray = camera.ray(x, y);
                        const auto i = y * Width + x;
                        if (ray[2] <= 0.f)
                        {
                            depth_pixels[i][0] = 1.f;
                            continue;
                        }
                        const float t = 8.f / ray[2];
                        const float wx = t * ray[0], wy = t * ray[1];
                        const bool panel = std::abs(wx) < .55f && std::abs(wy) < .45f;
                        const bool border = panel && (std::abs(wx) > .46f || std::abs(wy) > .37f);
                        const bool grille = panel && std::abs(wx) < .30f && wy < .08f && wy > -.29f &&
                                            std::cos(wx * 100.f) * std::cos(wy * 85.f) > .12f;
                        const float grain = .07f * std::cos(wx * 11.f + std::sin(wy * 18.f)) * std::sin(wy * 22.f);
                        wall[i] = border || grille ? Pixel{.29f, .25f, .18f, 1.f}
                                : panel ? Pixel{.79f, .70f, .50f, 1.f}
                                        : Pixel{.45f + grain, .36f + grain, .24f + grain, 1.f};
                        depth_pixels[i][0] = Camera::A + Camera::B / t;
                    }
                upload_pixels(scene_image, wall);
                upload_pixels(wall_depth, depth_pixels, frame ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_PREINITIALIZED);
                const auto sample = camera.screen({0, .15f, 8.f});
                const unsigned pixel = unsigned(sample[1]) * Width + unsigned(sample[0]);
                const unsigned area = camera.clipped_light_area(Lamp, Range);
                Frame legacy{};
                for (bool fixed : {false, true})
                {
                    LocalShadowBudget budget{2, 0, 1};
                    local_uniform.shadow_params[2] = fixed ? budget.reserve(true, true, dot(Lamp, Lamp), Range) >= 0
                                                           : area * 20 >= Width * Height;
                    write_local();
                    execute(local_fragment, push, false, weather_vs);
                    const float value = scene_pixels[pixel][0];
                    auto& minimum = fixed ? fixed_min : legacy_min;
                    auto& maximum = fixed ? fixed_max : legacy_max;
                    minimum = std::min(minimum, value);
                    maximum = std::max(maximum, value);
                    // Add the same neutral ambient background to both panels
                    // so the fixture's wall and switch stay visible in shadow.
                    for (unsigned i = 0; i < scene_pixels.size(); ++i)
                        for (unsigned channel = 0; channel < 3; ++channel)
                            scene_pixels[i][channel] += .32f * wall[i][channel] - .10f;
                    if (!fixed)
                        legacy = scene_pixels;
                    else
                        save_frame("light", frame, legacy, scene_pixels);
                }
            }
            if (legacy_max - legacy_min < .15f || fixed_max - fixed_min > .01f)
                throw std::runtime_error("camera turn did not reproduce shadow coverage flicker");
            std::cout << "Light, same wall panel during yaw: old " << legacy_min << ".." << legacy_max
                      << ", 0.9.191 " << fixed_min << ".." << fixed_max << '\n';

            check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
            std::memcpy(mapped, triangle.data(), sizeof(triangle));
            vkUnmapMemory(device, vertex_memory);
            update_texture(writes[0], input[0]);
            update_texture(writes[2], input[2]);
            for (const auto& asset : {atlas, wall_depth})
            {
                vkDestroyImageView(device, asset.view, nullptr);
                vkDestroyImage(device, asset.image, nullptr);
                vkFreeMemory(device, asset.memory, nullptr);
            }
        }
        vkDestroyShaderModule(device, water_ps, nullptr);
        vkDestroyShaderModule(device, water_vs, nullptr);
        vkDestroyImageView(device, field.view, nullptr);
        vkDestroyImage(device, field.image, nullptr);
        vkFreeMemory(device, field.memory, nullptr);
        vkDestroyShaderModule(device, composite_ps, nullptr);
        vkDestroyPipelineLayout(device, composite_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, composite_descriptor, nullptr);
        scene_push.padding[1] = 0;
        fill(memory, input[0], {127.f / 255, 127.f / 255, .1f, 1});

        // Negative control: the old opaque-only snapshot contains no water.
        scene_images[0].imageView = input[3].view;
        vkUpdateDescriptorSets(device, 1, &scene_writes[0], 0, nullptr);
        fill(memory, input[3], {.2f, .3f, .05f, 1});
        execute(distort_ps, scene_push, false, distort_vs);
        near(scene_pixels[48 * 128 + 64][2], .05f, "stale snapshot visibly replaces water with the opaque background");
        vkDestroyImageView(device, scene_image.view, nullptr);
        vkDestroyImage(device, scene_image.image, nullptr);
        vkFreeMemory(device, scene_image.memory, nullptr);
        vkDestroyShaderModule(device, instanced_vertex, nullptr);
        vkDestroyBuffer(device, instance_buffer, nullptr);
        vkFreeMemory(device, instance_memory, nullptr);
        vkDestroyShaderModule(device, local_fragment, nullptr);
        vkDestroyBuffer(device, local_buffer, nullptr);
        vkFreeMemory(device, local_memory, nullptr);
        vkDestroyPipelineLayout(device, local_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, local_layout, nullptr);
        vkDestroyImageView(device, shadow_array_view, nullptr);
        vkDestroySampler(device, environment_sampler, nullptr);
        std::cout << "surface channel readback: base/lightmap "
                     "sun+hemi/material/bump/aux/normal/detail/terrain/emission/environment2D/cube/particle OK\n";
        check(vkDeviceWaitIdle(device));
        for (auto module_handle : {vs, terrain, detail, flat, laemb, reflection, deferred_particle, additive_particle,
                                   additive_world_set, distort_vs, distort_ps, ui_vs, ui_ps, weather_vs, weather_ps, lod_vertex, lod_fragment})
            vkDestroyShaderModule(device, module_handle, nullptr);
        vkDestroyCommandPool(device, commands, nullptr);
        vkDestroyBuffer(device, readback, nullptr);
        vkFreeMemory(device, readback_memory, nullptr);
        vkDestroyBuffer(device, vertices, nullptr);
        vkFreeMemory(device, vertex_memory, nullptr);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyImageView(device, floor_depth.view, nullptr);
        vkDestroyImage(device, floor_depth.image, nullptr);
        vkFreeMemory(device, floor_depth.memory, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);
        vkDestroyBuffer(device, weather_uniform, nullptr);
        vkFreeMemory(device, weather_memory, nullptr);
        for (auto target : {weather_normal, weather_cube, weather_shadow})
        {
            vkDestroyImageView(device, target.view, nullptr);
            vkDestroyImage(device, target.image, nullptr);
            vkFreeMemory(device, target.memory, nullptr);
        }
        vkDestroyPipelineLayout(device, weather_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, weather_descriptor_layout, nullptr);
        vkDestroyBuffer(device, pose_buffer, nullptr);
        vkFreeMemory(device, pose_memory, nullptr);
        vkDestroyPipelineLayout(device, skinned_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, pose_layout, nullptr);
        vkDestroyDescriptorPool(device, pool, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptor_layout, nullptr);
        vkDestroySampler(device, sampler, nullptr);
        for (auto &target : output)
        {
            vkDestroyImageView(device, target.view, nullptr);
            vkDestroyImage(device, target.image, nullptr);
            vkFreeMemory(device, target.memory, nullptr);
        }
        for (auto &slot : input)
        {
            vkDestroyImageView(device, slot.view, nullptr);
            vkDestroyImage(device, slot.image, nullptr);
            vkFreeMemory(device, slot.memory, nullptr);
        }
        vkDestroyImageView(device, material_mips.view, nullptr);
        vkDestroyImage(device, material_mips.image, nullptr);
        vkFreeMemory(device, material_mips.memory, nullptr);
        vkDestroyShaderModule(device, forward_fragment, nullptr);
        vkDestroyShaderModule(device, forward_vertex, nullptr);
        vkDestroyPipelineLayout(device, forward_pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, forward_descriptor, nullptr);
        vkDestroyBuffer(device, forward_buffer, nullptr);
        vkFreeMemory(device, forward_memory, nullptr);
        vkDestroyImageView(device, cube.view, nullptr);
        vkDestroyImage(device, cube.image, nullptr);
        vkFreeMemory(device, cube.memory, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
