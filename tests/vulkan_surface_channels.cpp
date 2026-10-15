// Offscreen execution of the shipped fragment SPIR-V with 20 distinguishable
// sampled slots. Run with a software Vulkan ICD when no physical GPU exists.
#include <vulkan/vulkan.h>
#include <array>
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

static void check(VkResult r) { if (r != VK_SUCCESS) throw std::runtime_error("Vulkan result " + std::to_string(r)); }
static std::vector<uint32_t> shader(const std::string& name)
{
    std::ifstream input(name, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() < 20 || input.tellg() % 4) throw std::runtime_error("SPIR-V: " + name);
    std::vector<uint32_t> code(size_t(input.tellg()) / 4);
    input.seekg(0); input.read(reinterpret_cast<char*>(code.data()), std::streamsize(code.size() * 4));
    return code;
}
struct Memory
{
    VkDevice device{}; VkPhysicalDeviceMemoryProperties types{};
    uint32_t select(uint32_t bits, VkMemoryPropertyFlags required)
    {
        for (uint32_t i = 0; i < types.memoryTypeCount; ++i)
            if ((bits & (1u << i)) && (types.memoryTypes[i].propertyFlags & required) == required) return i;
        throw std::runtime_error("Vulkan memory type unavailable");
    }
    VkDeviceMemory allocate(VkMemoryRequirements requirements, VkMemoryPropertyFlags flags)
    {
        VkMemoryAllocateInfo info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        info.allocationSize = requirements.size;
        info.memoryTypeIndex = select(requirements.memoryTypeBits, flags);
        VkDeviceMemory result{}; check(vkAllocateMemory(device, &info, nullptr, &result)); return result;
    }
};
struct Image { VkImage image{}; VkImageView view{}; VkDeviceMemory memory{}; };
static Image image(Memory& memory, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
    VkMemoryPropertyFlags flags, uint32_t layers = 1)
{
    Image result;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D; info.format = format;
    info.extent = {1, 1, 1}; info.mipLevels = 1; info.arrayLayers = layers;
    if (layers == 6) info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    info.samples = VK_SAMPLE_COUNT_1_BIT; info.tiling = tiling; info.usage = usage;
    info.initialLayout = tiling == VK_IMAGE_TILING_LINEAR ?
        VK_IMAGE_LAYOUT_PREINITIALIZED : VK_IMAGE_LAYOUT_UNDEFINED;
    check(vkCreateImage(memory.device, &info, nullptr, &result.image));
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(memory.device, result.image, &requirements);
    result.memory = memory.allocate(requirements, flags);
    check(vkBindImageMemory(memory.device, result.image, result.memory, 0));
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = result.image;
    view.viewType = layers == 6 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = 1; view.subresourceRange.layerCount = layers;
    check(vkCreateImageView(memory.device, &view, nullptr, &result.view));
    return result;
}
static void fill(Memory& memory, const Image& image, const std::array<float, 4>& rgba,
    uint32_t layers = 1)
{
    void* mapped{}; check(vkMapMemory(memory.device, image.memory, 0, VK_WHOLE_SIZE, 0, &mapped));
    for (uint32_t face = 0; face < layers; ++face)
    {
        VkImageSubresource subresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, face};
        VkSubresourceLayout layout{};
        vkGetImageSubresourceLayout(memory.device, image.image, &subresource, &layout);
        std::memcpy(static_cast<uint8_t*>(mapped) + layout.offset, rgba.data(), sizeof(float) * 4);
    }
    vkUnmapMemory(memory.device, image.memory);
}
struct Vertex { float position[3], normal[3], uv[2], lightmap_uv[2], baked[4]; };
struct Push { float mvp[16]; float threshold{}, scale{}; float padding[2]{};
    float modes[4]{}, factors[4]{}, extra[4]{}, matrices[28]{}; };
static_assert(sizeof(Push) == 240);
static void near(float actual, float expected, const char* label)
{
    if (std::abs(actual - expected) > .015f)
        throw std::runtime_error(std::string(label) + " expected " + std::to_string(expected) +
            ", got " + std::to_string(actual));
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("usage: vulkan_surface_channels <vk shader directory>");
        VkInstance instance{}; VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        check(vkCreateInstance(&instance_info, nullptr, &instance));
        uint32_t count{}; check(vkEnumeratePhysicalDevices(instance, &count, nullptr));
        if (!count) throw std::runtime_error("no Vulkan physical device");
        std::vector<VkPhysicalDevice> devices(count); check(vkEnumeratePhysicalDevices(instance, &count, devices.data()));
        VkPhysicalDevice physical = devices[0];
        uint32_t families{}; vkGetPhysicalDeviceQueueFamilyProperties(physical, &families, nullptr);
        std::vector<VkQueueFamilyProperties> properties(families);
        vkGetPhysicalDeviceQueueFamilyProperties(physical, &families, properties.data());
        uint32_t family = 0;
        while (family < families && !(properties[family].queueFlags & VK_QUEUE_GRAPHICS_BIT)) ++family;
        if (family == families) throw std::runtime_error("no graphics queue");
        float priority = 1;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = family; queue_info.queueCount = 1; queue_info.pQueuePriorities = &priority;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = 1; device_info.pQueueCreateInfos = &queue_info;
        VkDevice device{}; check(vkCreateDevice(physical, &device_info, nullptr, &device));
        VkQueue queue{}; vkGetDeviceQueue(device, family, 0, &queue);
        Memory memory{device}; vkGetPhysicalDeviceMemoryProperties(physical, &memory.types);
        constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkFormatProperties support{}; vkGetPhysicalDeviceFormatProperties(physical, format, &support);
        if (!(support.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ||
            !(support.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
            throw std::runtime_error("float sampled/attachment images unsupported");
        std::array<Image, 20> input;
        for (auto& slot : input)
            slot = image(memory, format, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        std::array<Image, 3> output;
        for (auto& target : output)
            target = image(memory, format, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        // Each slot and each channel has a different value; neutral slots are
        // overridden for the variant under test below.
        std::array<std::array<float, 4>, 20> pixels{};
        for (uint32_t slot = 0; slot != 20; ++slot)
            pixels[slot] = {.03f + slot * .01f, .11f + slot * .012f,
                .19f + slot * .009f, .27f + slot * .011f};
        pixels[0] = {.25f, .4f, .6f, .8f};
        pixels[1] = {.5f, .63f, .75f, .9f};
        pixels[2] = {.15f, .3f, .45f, .6f};
        pixels[3] = {.2f, .3f, .4f, .5f};
        pixels[4] = {.4f, .2f, .7f, .5f};
        pixels[5] = {.6f, .5f, .2f, .5f};
        pixels[6] = {.8f, .7f, .5f, .5f};
        pixels[7] = {.3f, .4f, .5f, .6f};
        pixels[8] = {.1f, .2f, .7f, .4f};
        for (size_t i = 0; i < input.size(); ++i) fill(memory, input[i], pixels[i]);
        const Image cube = image(memory, format, VK_IMAGE_TILING_LINEAR,
            VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 6);
        const std::array<float, 4> cube_pixel{.91f, .22f, .34f, .67f};
        fill(memory, cube, cube_pixel, 6);
        VkSampler sampler{}; VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler_info.magFilter = sampler_info.minFilter = VK_FILTER_NEAREST;
        sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        check(vkCreateSampler(device, &sampler_info, nullptr, &sampler));
        std::array<VkDescriptorSetLayoutBinding, 20> bindings{};
        for (uint32_t i = 0; i < bindings.size(); ++i)
            bindings[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
        VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layout_info.bindingCount = bindings.size(); layout_info.pBindings = bindings.data();
        VkDescriptorSetLayout descriptor_layout{};
        check(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &descriptor_layout));
        VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 20};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool_info.maxSets = 1; pool_info.poolSizeCount = 1; pool_info.pPoolSizes = &pool_size;
        VkDescriptorPool pool{}; check(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = pool; alloc.descriptorSetCount = 1; alloc.pSetLayouts = &descriptor_layout;
        VkDescriptorSet set{}; check(vkAllocateDescriptorSets(device, &alloc, &set));
        std::array<VkDescriptorImageInfo, 20> images{};
        std::array<VkWriteDescriptorSet, 20> writes{};
        for (uint32_t i = 0; i < 20; ++i)
        {
            images[i] = {sampler, input[i].view, VK_IMAGE_LAYOUT_GENERAL};
            writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writes[i].dstSet = set; writes[i].dstBinding = i; writes[i].descriptorCount = 1;
            writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writes[i].pImageInfo = &images[i];
        }
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
        VkPushConstantRange range{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(Push)};
        VkPipelineLayoutCreateInfo pipeline_layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pipeline_layout_info.setLayoutCount = 1; pipeline_layout_info.pSetLayouts = &descriptor_layout;
        pipeline_layout_info.pushConstantRangeCount = 1; pipeline_layout_info.pPushConstantRanges = &range;
        VkPipelineLayout pipeline_layout{};
        check(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));
        std::array<VkAttachmentDescription, 3> attachments{};
        std::array<VkAttachmentReference, 3> references{};
        for (uint32_t i = 0; i < 3; ++i)
        {
            attachments[i].format = format; attachments[i].samples = VK_SAMPLE_COUNT_1_BIT;
            attachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[i].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            references[i] = {i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        }
        VkSubpassDescription subpass{}; subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 3; subpass.pColorAttachments = references.data();
        VkRenderPassCreateInfo rp_info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rp_info.attachmentCount = 3; rp_info.pAttachments = attachments.data();
        rp_info.subpassCount = 1; rp_info.pSubpasses = &subpass;
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
        rp_info.dependencyCount = 2; rp_info.pDependencies = dependencies;
        VkRenderPass render_pass{}; check(vkCreateRenderPass(device, &rp_info, nullptr, &render_pass));
        std::array<VkImageView, 3> views{output[0].view, output[1].view, output[2].view};
        VkFramebufferCreateInfo framebuffer_info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        framebuffer_info.renderPass = render_pass; framebuffer_info.attachmentCount = 3;
        framebuffer_info.pAttachments = views.data(); framebuffer_info.width = framebuffer_info.height = 1;
        framebuffer_info.layers = 1;
        VkFramebuffer framebuffer{}; check(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffer));
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = sizeof(Vertex) * 3; buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkBuffer vertices{}; check(vkCreateBuffer(device, &buffer_info, nullptr, &vertices));
        VkMemoryRequirements required{}; vkGetBufferMemoryRequirements(device, vertices, &required);
        VkDeviceMemory vertex_memory = memory.allocate(required,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, vertices, vertex_memory, 0));
        std::array<Vertex, 3> triangle{};
        triangle[0].position[0] = -1; triangle[0].position[1] = 1;
        triangle[1].position[0] = 3; triangle[1].position[1] = 1;
        triangle[2].position[0] = -1; triangle[2].position[1] = -3;
        for (auto& vertex : triangle)
        { vertex.normal[2] = 1; vertex.uv[0] = vertex.uv[1] = .5f;
          vertex.lightmap_uv[0] = vertex.lightmap_uv[1] = .5f;
          vertex.baked[0] = .5f; vertex.baked[1] = .25f;
          vertex.baked[2] = .75f; vertex.baked[3] = 1.f; }
        void* mapped{}; check(vkMapMemory(device, vertex_memory, 0, VK_WHOLE_SIZE, 0, &mapped));
        std::memcpy(mapped, triangle.data(), sizeof(triangle)); vkUnmapMemory(device, vertex_memory);
        buffer_info.size = 3 * 16; buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VkBuffer readback{}; check(vkCreateBuffer(device, &buffer_info, nullptr, &readback));
        vkGetBufferMemoryRequirements(device, readback, &required);
        VkDeviceMemory readback_memory = memory.allocate(required,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        check(vkBindBufferMemory(device, readback, readback_memory, 0));
        VkVertexInputBindingDescription vertex_binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        VkVertexInputAttributeDescription attributes[]{
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
            {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
            {5, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, lightmap_uv)},
            {6, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, baked)}};
        VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        vertex_input.vertexBindingDescriptionCount = 1; vertex_input.pVertexBindingDescriptions = &vertex_binding;
        vertex_input.vertexAttributeDescriptionCount = 5; vertex_input.pVertexAttributeDescriptions = attributes;
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkViewport viewport{0, 0, 1, 1, 0, 1}; VkRect2D scissor{{0, 0}, {1, 1}};
        VkPipelineViewportStateCreateInfo viewport_state{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        viewport_state.viewportCount = 1; viewport_state.pViewports = &viewport;
        viewport_state.scissorCount = 1; viewport_state.pScissors = &scissor;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        raster.polygonMode = VK_POLYGON_MODE_FILL; raster.cullMode = VK_CULL_MODE_NONE; raster.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend_attachment{};
        blend_attachment.colorWriteMask = 15;
        std::array<VkPipelineColorBlendAttachmentState, 3> color_blends{}; color_blends.fill(blend_attachment);
        VkPipelineColorBlendStateCreateInfo blend_state{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        blend_state.attachmentCount = 3; blend_state.pAttachments = color_blends.data();
        VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        VkGraphicsPipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
        pipeline_info.pVertexInputState = &vertex_input; pipeline_info.pInputAssemblyState = &assembly;
        pipeline_info.pViewportState = &viewport_state; pipeline_info.pRasterizationState = &raster;
        pipeline_info.pMultisampleState = &multisample; pipeline_info.pDepthStencilState = &depth;
        pipeline_info.pColorBlendState = &blend_state; pipeline_info.layout = pipeline_layout;
        pipeline_info.renderPass = render_pass;
        VkCommandPoolCreateInfo cmd_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        cmd_pool_info.queueFamilyIndex = family;
        VkCommandPool commands{}; check(vkCreateCommandPool(device, &cmd_pool_info, nullptr, &commands));
        VkCommandBufferAllocateInfo cmd_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmd_info.commandPool = commands; cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cmd_info.commandBufferCount = 1;
        VkCommandBuffer cmd{}; check(vkAllocateCommandBuffers(device, &cmd_info, &cmd));
        auto module = [&](const std::string& filename) {
            auto code = shader(std::string(argv[1]) + "/" + filename);
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            info.codeSize = code.size() * 4; info.pCode = code.data();
            VkShaderModule result{}; check(vkCreateShaderModule(device, &info, nullptr, &result)); return result;
        };
        const auto vs = module("level_terrain.vs.spv");
        const auto terrain = module("level_terrain.ps.spv");
        const auto detail = module("level_detail.ps.spv");
        const auto flat = module("level_lightmap.ps.spv");
        const auto laemb = module("level_laemb_base.ps.spv");
        const auto reflection = module("level_environment.ps.spv");
        const auto deferred_particle = module("particle_world_deferred.ps.spv");
        bool inputs_initialized = false;
        bool cube_initialized = false;
        auto execute = [&](VkShaderModule fragment, const Push& push, bool use_cube = false) {
            VkPipelineShaderStageCreateInfo stages[2]{};
            for (auto& stage : stages) stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT; stages[0].module = vs; stages[0].pName = "main";
            stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; stages[1].module = fragment; stages[1].pName = "main";
            pipeline_info.stageCount = 2; pipeline_info.pStages = stages;
            VkPipeline pipeline{}; check(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline));
            check(vkResetCommandPool(device, commands, 0));
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; check(vkBeginCommandBuffer(cmd, &begin));
            if (use_cube && !cube_initialized)
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
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
                cube_initialized = true;
            }
            if (!inputs_initialized)
            {
                std::array<VkImageMemoryBarrier, 20> barriers{};
                for (size_t i = 0; i < barriers.size(); ++i)
                {
                    barriers[i] = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                    barriers[i].oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
                    barriers[i].newLayout = VK_IMAGE_LAYOUT_GENERAL;
                    barriers[i].image = input[i].image;
                    barriers[i].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                    barriers[i].subresourceRange.levelCount = barriers[i].subresourceRange.layerCount = 1;
                    barriers[i].srcQueueFamilyIndex = barriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    barriers[i].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
                    barriers[i].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                }
                vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0, 0, nullptr, 0, nullptr, barriers.size(), barriers.data());
                inputs_initialized = true;
            }
            std::array<VkClearValue, 3> clear{};
            VkRenderPassBeginInfo rp{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            rp.renderPass = render_pass; rp.framebuffer = framebuffer; rp.renderArea.extent = {1, 1};
            rp.clearValueCount = 3; rp.pClearValues = clear.data();
            vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &set, 0, nullptr);
            VkDeviceSize offset{}; vkCmdBindVertexBuffers(cmd, 0, 1, &vertices, &offset);
            vkCmdPushConstants(cmd, pipeline_layout, range.stageFlags, 0, sizeof(push), &push);
            vkCmdDraw(cmd, 3, 1, 0, 0); vkCmdEndRenderPass(cmd);
            std::array<VkBufferImageCopy, 3> copies{};
            for (uint32_t i = 0; i < 3; ++i)
            {
                copies[i].bufferOffset = i * 16;
                copies[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copies[i].imageSubresource.layerCount = 1;
                copies[i].imageExtent = {1, 1, 1};
            }
            for (uint32_t i = 0; i < 3; ++i)
                vkCmdCopyImageToBuffer(cmd, output[i].image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    readback, 1, &copies[i]);
            VkBufferMemoryBarrier host{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
            host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
            host.buffer = readback; host.size = 48;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
                0, 0, nullptr, 1, &host, 0, nullptr);
            check(vkEndCommandBuffer(cmd));
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount = 1; submit.pCommandBuffers = &cmd;
            check(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE)); check(vkQueueWaitIdle(queue));
            std::array<std::array<float, 4>, 3> result{};
            void* data{}; check(vkMapMemory(device, readback_memory, 0, VK_WHOLE_SIZE, 0, &data));
            std::memcpy(result.data(), data, sizeof(result)); vkUnmapMemory(device, readback_memory);
            vkDestroyPipeline(device, pipeline, nullptr); return result;
        };
        Push push{}; for (int i = 0; i < 16; ++i) push.mvp[i] = (i % 5 == 0 ? 1.f : 0.f);
        for (int index : {0, 5, 8, 13, 16, 21, 26}) push.matrices[index] = 1.f;
        push.scale = 1; push.modes[3] = 1; push.factors[1] = 1;
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
        push.modes[2] = 1;
        auto normal = execute(flat, push);
        if (std::abs(normal[1][0] - bump[1][0]) < .02f)
            throw std::runtime_error("external normal slot 15 did not change output normal");
        push.modes[2] = 0; push.modes[1] = 0;
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
        push.modes[0] = 0; push.modes[1] = 0;
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
        near(reflected_cube[0][0], cube_pixel[0] * (1 - pixels[0][3]) +
            pixels[0][0] * pixels[0][3], "reflection cube red");
        near(reflected_cube[0][1], cube_pixel[1] * (1 - pixels[0][3]) +
            pixels[0][1] * pixels[0][3], "reflection cube green");
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
        std::cout << "surface channel readback: base/lightmap sun+hemi/material/bump/aux/normal/detail/terrain/emission/environment2D/cube/particle OK\n";
        check(vkDeviceWaitIdle(device));
        for (auto module_handle : {vs, terrain, detail, flat, laemb, reflection, deferred_particle})
            vkDestroyShaderModule(device, module_handle, nullptr);
        vkDestroyCommandPool(device, commands, nullptr);
        vkDestroyBuffer(device, readback, nullptr); vkFreeMemory(device, readback_memory, nullptr);
        vkDestroyBuffer(device, vertices, nullptr); vkFreeMemory(device, vertex_memory, nullptr);
        vkDestroyFramebuffer(device, framebuffer, nullptr);
        vkDestroyRenderPass(device, render_pass, nullptr);
        vkDestroyDescriptorPool(device, pool, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptor_layout, nullptr);
        vkDestroySampler(device, sampler, nullptr);
        for (auto& target : output)
        {
            vkDestroyImageView(device, target.view, nullptr);
            vkDestroyImage(device, target.image, nullptr);
            vkFreeMemory(device, target.memory, nullptr);
        }
        for (auto& slot : input)
        {
            vkDestroyImageView(device, slot.view, nullptr);
            vkDestroyImage(device, slot.image, nullptr);
            vkFreeMemory(device, slot.memory, nullptr);
        }
        vkDestroyImageView(device, cube.view, nullptr);
        vkDestroyImage(device, cube.image, nullptr);
        vkFreeMemory(device, cube.memory, nullptr);
        vkDestroyDevice(device, nullptr); vkDestroyInstance(instance, nullptr);
        return 0;
    }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
