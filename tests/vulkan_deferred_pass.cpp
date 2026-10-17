#include "src/Layers/xrRenderVK/DeferredPass.h"
#include "src/Layers/xrRenderVK/SunShadowTargets.h"
#include "src/Layers/xrRenderVK/ModelGeometry.h"
#include "src/Layers/xrRenderVK/GameShaderResources.h"
#include "src/Layers/xrRenderVK/LevelPassGraph.h"

#include <cassert>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

using namespace xray::render::vulkan;

namespace
{
template <typename T> T handle(uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}

uint32_t pipeline_count{}, draw_count{}, descriptor_updates{}, descriptor_frees{};
uint32_t pipeline_destroys{};
uint32_t expected_first_index{};
int32_t expected_vertex_base{};
bool weather_bind{}, weather_push{};
bool cutout_alpha_push{}, cutout_scale_push{}, forward_detail_scale_push{};
SurfaceParameters expected_surface{};
bool surface_push{};
bool environment_camera_push{}, environment_surface_push{};
bool world_alpha_push{};
float expected_world_alpha_ref{.25f};
bool distinct_surface_images{}, surface_bindings_checked{};
float expected_environment_camera[4]{2.f, 3.f, 4.f, 1.f};
VkPipeline bound_pipeline{};
VkPipelineLayout geometry_layout{}, skinned_layout{}, lighting_layout{}, weather_layout{};
VkDescriptorSet updated_material{};
VkImageView updated_albedo{};
VkSampler updated_sampler{};
float expected_mvp[16]{};

VkResult VKAPI_PTR create_layout(VkDevice, const VkPipelineLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkPipelineLayout* output)
{
    assert(info->setLayoutCount >= 1 && info->setLayoutCount <= 3);
    if (!info->pushConstantRangeCount)
    {
        assert(info->setLayoutCount == 2);
        *output = handle<VkPipelineLayout>(45);
        return VK_SUCCESS;
    }
    if (info->pushConstantRangeCount == 2)
    {
        assert(info->setLayoutCount == 2 &&
            info->pPushConstantRanges[0].size == sizeof(expected_mvp) &&
            info->pPushConstantRanges[1].offset == sizeof(expected_mvp));
        *output = handle<VkPipelineLayout>(46);
        return VK_SUCCESS;
    }
    assert(info->pushConstantRangeCount == 1);
    const auto& range = info->pPushConstantRanges[0];
    if (range.stageFlags & VK_SHADER_STAGE_VERTEX_BIT)
    {
        assert(range.stageFlags == (VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
        assert(range.size == sizeof(float) * 60);
        *output = info->setLayoutCount == 3 ?
            (skinned_layout = handle<VkPipelineLayout>(44)) :
            (geometry_layout = handle<VkPipelineLayout>(41));
    }
    else
    {
        assert(range.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT);
        if (info->setLayoutCount == 2)
        {
            assert(range.size == sizeof(WeatherLighting));
            *output = weather_layout = handle<VkPipelineLayout>(43);
        }
        else
        {
            assert(range.size == sizeof(DeferredLight));
            *output = lighting_layout = handle<VkPipelineLayout>(42);
        }
    }
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_layout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) {}

VkResult VKAPI_PTR create_pipeline(VkDevice, VkPipelineCache, uint32_t count,
    const VkGraphicsPipelineCreateInfo* info, const VkAllocationCallbacks*, VkPipeline* output)
{
    assert(count == 1 && info->stageCount == 2);
    if (info->pStages[1].module == handle<VkShaderModule>(103) ||
        info->pStages[1].module == handle<VkShaderModule>(105))
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->layout == (info->pStages[1].module == handle<VkShaderModule>(105) ?
            skinned_layout : geometry_layout));
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable);
        assert(info->pColorBlendState->attachmentCount == 3 &&
            !info->pColorBlendState->pAttachments[0].blendEnable);
    }
    else if (uintptr_t(info->pStages[1].module) >= 107 &&
        uintptr_t(info->pStages[1].module) <= 109)
    {
        assert(info->renderPass == handle<VkRenderPass>(11) && info->layout == geometry_layout);
        assert(info->pDepthStencilState->depthTestEnable &&
            !info->pDepthStencilState->depthWriteEnable);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(info->pColorBlendState->attachmentCount == 1 && color.blendEnable);
        const auto module = uintptr_t(info->pStages[1].module);
        assert(color.srcColorBlendFactor == (module == 107 ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_DST_COLOR));
        assert(color.dstColorBlendFactor == (module == 107 ? VK_BLEND_FACTOR_ONE :
            module == 108 ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_SRC_COLOR));
    }
    else if (uintptr_t(info->pStages[1].module) >= 110 &&
        uintptr_t(info->pStages[1].module) <= 113)
    {
        assert(info->renderPass == handle<VkRenderPass>(11) && info->layout == geometry_layout);
        assert(info->pDepthStencilState->depthTestEnable &&
            !info->pDepthStencilState->depthWriteEnable);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(info->pColorBlendState->attachmentCount == 1);
        const bool replace = info->pStages[1].module == handle<VkShaderModule>(110);
        assert(color.blendEnable == (replace ? VK_FALSE : VK_TRUE));
        if (!replace)
            assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
                color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(114))
    {
        assert(info->renderPass == handle<VkRenderPass>(11) && info->layout == geometry_layout);
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable);
        assert(info->pColorBlendState->attachmentCount == 1 &&
            !info->pColorBlendState->pAttachments[0].blendEnable);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(116))
    {
        assert(info->renderPass == handle<VkRenderPass>(11) && info->layout == skinned_layout);
        assert(info->pVertexInputState->pVertexBindingDescriptions[0].stride == sizeof(ModelVertex));
        assert(info->pDepthStencilState->depthTestEnable &&
            !info->pDepthStencilState->depthWriteEnable);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(color.blendEnable && color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
    else if (uintptr_t(info->pStages[1].module) >= 117 &&
        uintptr_t(info->pStages[1].module) <= 126)
    {
        const unsigned mode = unsigned(uintptr_t(info->pStages[1].module) - 117);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(info->renderPass == handle<VkRenderPass>(11) &&
            info->pColorBlendState->attachmentCount == 1);
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable == (mode == 0));
        assert(color.blendEnable == (mode != 0 && mode != 7));
        if (mode == 2) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_ONE &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
        if (mode == 3 || mode == 4 || mode == 6)
            assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_DST_COLOR &&
                color.dstColorBlendFactor == (mode == 3 ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_SRC_COLOR));
        if (mode == 5) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
    }
    else if (uintptr_t(info->pStages[1].module) >= 128 &&
        uintptr_t(info->pStages[1].module) <= 147)
    {
        const unsigned module = unsigned(uintptr_t(info->pStages[1].module));
        const unsigned mode = module >= 138 ? module - 138 : module - 128;
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(info->renderPass == handle<VkRenderPass>(11) &&
            info->layout == (module >= 138 ? skinned_layout : geometry_layout));
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable == (mode == 0));
        assert(color.blendEnable == (mode != 0 && mode != 7));
        if (mode == 2) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_ONE &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
        if (mode == 3 || mode == 4 || mode == 6)
            assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_DST_COLOR &&
                color.dstColorBlendFactor == (mode == 3 ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_SRC_COLOR));
        if (mode == 5) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
    }
    else if (uintptr_t(info->pStages[1].module) >= 148 &&
        uintptr_t(info->pStages[1].module) <= 153)
    {
        const unsigned mode = unsigned(uintptr_t(info->pStages[1].module) - 148);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(info->renderPass == handle<VkRenderPass>(11) &&
            info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable == (mode == 0));
        assert(color.blendEnable == (mode != 0));
        if (mode == 2) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_ONE &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
        if (mode == 3 || mode == 4)
            assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_DST_COLOR &&
                color.dstColorBlendFactor == (mode == 3 ? VK_BLEND_FACTOR_ZERO : VK_BLEND_FACTOR_SRC_COLOR));
        if (mode == 5) assert(color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(127))
    {
        assert(info->renderPass == handle<VkRenderPass>(11) &&
            info->pColorBlendState->attachmentCount == 1 &&
            info->pColorBlendState->pAttachments[0].blendEnable);
        assert(info->pDepthStencilState->depthTestEnable &&
            !info->pDepthStencilState->depthWriteEnable);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(95) ||
        info->pStages[1].module == handle<VkShaderModule>(101))
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->layout == geometry_layout);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE &&
            info->pDepthStencilState->depthWriteEnable == VK_TRUE);
        assert(info->pColorBlendState->attachmentCount == 3 &&
            info->pColorBlendState->pAttachments[0].blendEnable == VK_FALSE);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(97) ||
        info->pStages[1].module == handle<VkShaderModule>(99))
    {
        assert(info->renderPass == handle<VkRenderPass>(11));
        assert(info->layout == geometry_layout);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE &&
            info->pDepthStencilState->depthWriteEnable == VK_FALSE);
        assert(info->pColorBlendState->attachmentCount == 1);
        const auto& color = info->pColorBlendState->pAttachments[0];
        assert(color.blendEnable == VK_TRUE &&
            color.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA &&
            color.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(91))
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE);
        assert(info->pDepthStencilState->depthWriteEnable == VK_FALSE);
        assert(info->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_LESS_OR_EQUAL);
        assert(info->pColorBlendState->attachmentCount == 3);
        const auto* attachments = info->pColorBlendState->pAttachments;
        assert(attachments[0].blendEnable == VK_TRUE);
        assert(attachments[0].srcColorBlendFactor == VK_BLEND_FACTOR_DST_COLOR);
        assert(attachments[0].dstColorBlendFactor == VK_BLEND_FACTOR_SRC_COLOR);
        assert(attachments[0].colorWriteMask == (VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT));
        assert(attachments[1].colorWriteMask == 0);
        assert(attachments[2].colorWriteMask == 0);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(201))
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable);
        assert(info->pColorBlendState->attachmentCount == 3 &&
            !info->pColorBlendState->pAttachments[0].blendEnable);
    }
    else if (info->pStages[1].module == handle<VkShaderModule>(202))
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->pDepthStencilState->depthTestEnable &&
            info->pDepthStencilState->depthWriteEnable);
        assert(!info->pColorBlendState->pAttachments[0].blendEnable);
    }
    else if (pipeline_count < 2)
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->pVertexInputState->vertexAttributeDescriptionCount == 5);
        const auto* attributes = info->pVertexInputState->pVertexAttributeDescriptions;
        assert(attributes[0].format == VK_FORMAT_R32G32B32_SFLOAT &&
            attributes[0].offset == offsetof(LevelVertex, position));
        assert(attributes[1].format == VK_FORMAT_R32G32B32A32_SFLOAT &&
            attributes[1].offset == offsetof(LevelVertex, normal));
        assert(attributes[2].format == VK_FORMAT_R32G32_SFLOAT &&
            attributes[2].offset == offsetof(LevelVertex, uv));
        assert(attributes[3].location == 5 && attributes[3].offset == offsetof(LevelVertex, lightmap_uv));
        assert(attributes[4].location == 6 && attributes[4].offset == offsetof(LevelVertex, baked));
        assert(info->pColorBlendState->attachmentCount == 3);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE);
        assert(info->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_LESS);
        assert(info->pDepthStencilState->depthWriteEnable == VK_TRUE);
        assert(info->pColorBlendState->pAttachments[0].blendEnable == VK_FALSE);
    }
    else if (pipeline_count == 2 || pipeline_count == 3)
    {
        assert(info->pStages[1].module == handle<VkShaderModule>(18));
        assert(info->renderPass == handle<VkRenderPass>(11));
        assert(info->pColorBlendState->attachmentCount == 1);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE);
        assert(info->pDepthStencilState->depthWriteEnable == (pipeline_count == 3 ? VK_TRUE : VK_FALSE));
        assert(info->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_LESS);
        assert(info->pColorBlendState->pAttachments[0].blendEnable == VK_TRUE);
    }
    else if (pipeline_count < 6)
    {
        assert((pipeline_count == 4 || pipeline_count == 5) &&
            info->renderPass == handle<VkRenderPass>(11));
        assert(info->pVertexInputState->vertexBindingDescriptionCount == 0);
        assert(info->pColorBlendState->attachmentCount == 1);
    }
    else
    {
        const bool skinned = pipeline_count == 8 || (pipeline_count >= 13 && pipeline_count <= 16);
        assert(info->pVertexInputState->vertexAttributeDescriptionCount == 5u);
        if (skinned)
        {
            assert(info->layout == skinned_layout);
            assert(info->pVertexInputState->pVertexBindingDescriptions[0].stride == sizeof(ModelVertex));
            assert(info->pVertexInputState->pVertexAttributeDescriptions[3].format == VK_FORMAT_R16G16B16A16_UINT);
        }
        assert(info->pStages[0].module == handle<VkShaderModule>(50 + 2 * (pipeline_count - 6)));
        assert(info->pStages[1].module == handle<VkShaderModule>(51 + 2 * (pipeline_count - 6)));
        const bool present = pipeline_count == 7 || pipeline_count == 16;
        assert(info->renderPass == handle<VkRenderPass>(present ? 11 : 10));
        assert(info->pColorBlendState->pAttachments[0].blendEnable == (present ? VK_TRUE : VK_FALSE));
        assert(info->pRasterizationState->cullMode == VK_CULL_MODE_NONE);
        if (pipeline_count == 11 || pipeline_count == 12 || (pipeline_count >= 13 && pipeline_count <= 15))
            assert(info->pDepthStencilState->depthWriteEnable == VK_TRUE);
        if (pipeline_count == 16)
            assert(info->pDepthStencilState->depthWriteEnable == VK_TRUE);
    }
    *output = handle<VkPipeline>(100 + pipeline_count++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*) { ++pipeline_destroys; }
void VKAPI_PTR bind_pipeline(VkCommandBuffer, VkPipelineBindPoint, VkPipeline pipeline)
{ bound_pipeline = pipeline; }
void VKAPI_PTR viewport(VkCommandBuffer, uint32_t, uint32_t, const VkViewport* value)
{ assert(value->width == 640 && value->height == 480); }
void VKAPI_PTR scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D* value)
{ assert(value->extent.width == 640 && value->extent.height == 480); }
void VKAPI_PTR bind_vertices(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer* value,
    const VkDeviceSize*)
{ assert(*value == handle<VkBuffer>(20)); }
void VKAPI_PTR bind_indices(VkCommandBuffer, VkBuffer value, VkDeviceSize, VkIndexType type)
{ assert(value == handle<VkBuffer>(21) && type == VK_INDEX_TYPE_UINT32); }
void VKAPI_PTR push_constants(VkCommandBuffer, VkPipelineLayout layout, VkShaderStageFlags stage,
    uint32_t offset, uint32_t size, const void* data)
{
    if (layout == weather_layout)
    {
        assert(stage == VK_SHADER_STAGE_FRAGMENT_BIT && size == sizeof(WeatherLighting));
        weather_push = true;
        return;
    }
    assert(layout == geometry_layout || layout == skinned_layout);
    if (stage == VK_SHADER_STAGE_FRAGMENT_BIT)
    {
        if (offset == 128)
        {
            assert(size == sizeof(float) * 28);
            const float* rows = static_cast<const float*>(data);
            assert(rows[0] == 1.f && rows[5] == 1.f && rows[8] == 1.f &&
                rows[13] == 1.f && rows[16] == 1.f && rows[21] == 1.f && rows[26] == 1.f);
            return;
        }
        if (offset == 96)
        {
            assert(size == sizeof(SurfaceParameters) &&
                std::memcmp(data, &expected_surface, sizeof(expected_surface)) == 0);
            environment_surface_push = true;
            return;
        }
        if (offset == 64 && size == sizeof(expected_environment_camera))
        {
            assert(std::memcmp(data, expected_environment_camera, size) == 0);
            environment_camera_push = true;
            return;
        }
        if (offset == 64 && size == sizeof(float) && uintptr_t(bound_pipeline) >= 128)
        {
            assert(*static_cast<const float*>(data) == expected_world_alpha_ref);
            world_alpha_push = true;
        }
        if (offset == 80)
        {
            assert(size == sizeof(SurfaceParameters) &&
                std::memcmp(data, &expected_surface, sizeof(expected_surface)) == 0);
            surface_push = true;
            return;
        }
        assert((offset == sizeof(expected_mvp) || offset == sizeof(expected_mvp) + sizeof(float)) &&
            size == sizeof(float));
        if (bound_pipeline == handle<VkPipeline>(119) ||
            bound_pipeline == handle<VkPipeline>(122))
        {
            float value;
            std::memcpy(&value, data, sizeof(value));
            if (offset == sizeof(expected_mvp))
            { assert(std::abs(value - .4f) < 1e-6f); cutout_alpha_push = true; }
            else
            { assert(std::abs(value - 2.5f) < 1e-6f); cutout_scale_push = true; }
        }
        if ((bound_pipeline == handle<VkPipeline>(120) ||
                bound_pipeline == handle<VkPipeline>(121)) &&
            offset == sizeof(expected_mvp) + sizeof(float))
        {
            float value;
            std::memcpy(&value, data, sizeof(value));
            assert(std::abs(value - 1.75f) < 1e-6f || std::abs(value - 1.f) < 1e-6f);
            if (std::abs(value - 1.75f) < 1e-6f) forward_detail_scale_push = true;
        }
        return;
    }
    assert(stage == VK_SHADER_STAGE_VERTEX_BIT && offset == 0 && size == sizeof(expected_mvp));
    assert(std::memcmp(data, expected_mvp, sizeof(expected_mvp)) == 0);
}
void VKAPI_PTR draw_indexed(VkCommandBuffer, uint32_t count, uint32_t instances,
    uint32_t first_index, int32_t vertex_base, uint32_t)
{
    assert(count == 6 && instances == 1);
    assert(first_index == expected_first_index);
    assert(vertex_base == expected_vertex_base);
    constexpr uintptr_t expected_pipelines[]{100, 106, 108, 101, 102, 107, 103, 119,
        120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133,
        134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
        145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155,
        156, 157, 158, 159, 160, 161, 162, 163, 164, 165,
        166, 167, 168, 169, 170, 171, 118, 172};
    assert(draw_count < std::size(expected_pipelines));
    if (bound_pipeline != handle<VkPipeline>(expected_pipelines[draw_count]))
        std::fprintf(stderr, "draw=%u expected pipeline=%zu actual=%zu\n", draw_count,
            size_t(expected_pipelines[draw_count]), size_t(uintptr_t(bound_pipeline)));
    assert(bound_pipeline == handle<VkPipeline>(expected_pipelines[draw_count]));
    ++draw_count;
}
void VKAPI_PTR draw(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t) {}

VkResult VKAPI_PTR create_descriptor_layout(VkDevice, const VkDescriptorSetLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorSetLayout* output)
{
    assert(info->bindingCount >= 1 && info->bindingCount <= 20);
    assert(info->pBindings[0].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
        info->pBindings[0].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
        info->pBindings[0].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    if (info->bindingCount >= 2)
        assert(info->pBindings[1].binding == 1);
    if (info->bindingCount == 5)
        assert(info->pBindings[4].binding == 4 &&
            info->pBindings[4].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    *output = handle<VkDescriptorSetLayout>(info->pBindings[0].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ?
        39 : 30 + info->bindingCount);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_descriptor_layout(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*) {}
VkResult VKAPI_PTR create_pool(VkDevice, const VkDescriptorPoolCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorPool* output)
{
    assert(info->maxSets == 8192 && info->poolSizeCount == 3 &&
        info->pPoolSizes[0].descriptorCount == 163840 &&
        info->pPoolSizes[1].type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER &&
        info->pPoolSizes[2].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    *output = handle<VkDescriptorPool>(33);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pool(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*) {}
VkResult VKAPI_PTR allocate_sets(VkDevice, const VkDescriptorSetAllocateInfo* info,
    VkDescriptorSet* output)
{
    assert(info->descriptorPool == handle<VkDescriptorPool>(33));
    if (*info->pSetLayouts == handle<VkDescriptorSetLayout>(39))
        *output = handle<VkDescriptorSet>(35);
    else
        *output = updated_material = handle<VkDescriptorSet>(34);
    return VK_SUCCESS;
}
VkResult VKAPI_PTR free_sets(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*)
{ ++descriptor_frees; return VK_SUCCESS; }
void VKAPI_PTR update_sets(VkDevice, uint32_t count, const VkWriteDescriptorSet* writes,
    uint32_t, const VkCopyDescriptorSet*)
{
    assert(count == 1 || count == 2 || count == 4 || count == 6 || count == 7 || count == 13);
    if (writes[0].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        assert(count == 1 && writes[0].pBufferInfo->buffer == handle<VkBuffer>(23));
        return;
    }
    if (writes[0].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        assert(count == 1 && writes[0].pBufferInfo->buffer == handle<VkBuffer>(24) &&
            writes[0].pBufferInfo->range == sizeof(ForwardLightUniform));
        return;
    }
    if (count == 2 && writes[0].dstBinding == 3)
    {
        assert(writes[1].dstBinding == 4 &&
            writes[1].pBufferInfo->range == sizeof(SunShadowUniform));
        return;
    }
    assert(writes[0].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    assert(writes[0].pImageInfo->imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
        ((writes[0].dstBinding == 0 || writes[0].dstBinding >= 7 ||
            (count == 6 && writes[0].pImageInfo->imageView == handle<VkImageView>(60))) &&
            writes[0].pImageInfo->imageLayout == VK_IMAGE_LAYOUT_GENERAL) ||
        ((writes[0].dstBinding == 0 || writes[0].dstBinding == 3) &&
            writes[0].pImageInfo->imageLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));
    if (count == 6 && writes[0].pImageInfo->imageView == handle<VkImageView>(60))
    {
        for (uint32_t i = 0; i < count; ++i)
            assert(writes[i].dstBinding == i && writes[i].pImageInfo->imageView == handle<VkImageView>(60 + i));
    }
    if (count == 7)
        for (uint32_t i = 0; i < count; ++i)
            assert(writes[i].dstBinding == i);
    if (count == 13)
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            assert(writes[i].dstBinding == i + 7);
            if (distinct_surface_images)
                assert(writes[i].pImageInfo->imageView == handle<VkImageView>(201 + i));
        }
        if (distinct_surface_images) surface_bindings_checked = true;
    }
    updated_albedo = writes[0].pImageInfo->imageView;
    updated_sampler = writes[0].pImageInfo->sampler;
    ++descriptor_updates;
}
void VKAPI_PTR bind_sets(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout layout,
    uint32_t first_set, uint32_t count, const VkDescriptorSet* sets, uint32_t, const uint32_t*)
{
    if (layout == weather_layout)
    {
        assert(count == 2 && sets[0] && sets[1]);
        weather_bind = true;
        return;
    }
    if (layout == skinned_layout)
    {
        if (first_set == 2) assert(count == 1 && sets[0]);
        else assert(first_set == 0 && count == 2 && sets[0] == updated_material && sets[1]);
        return;
    }
    assert(layout == geometry_layout && count == 1 && *sets == updated_material);
}
}

static VkResult VKAPI_PTR create_render_pass(VkDevice, const VkRenderPassCreateInfo* info,
    const VkAllocationCallbacks*, VkRenderPass* result)
{
    assert(info->attachmentCount == 4 && info->subpassCount == 1);
    assert(info->pAttachments[0].format == VK_FORMAT_R8G8B8A8_UNORM);
    assert(info->pAttachments[1].format == VK_FORMAT_R8G8B8A8_UNORM);
    assert(info->pAttachments[2].format == VK_FORMAT_R8G8B8A8_UNORM);
    assert(info->pAttachments[3].format == VK_FORMAT_D24_UNORM_S8_UINT);
    assert(info->pAttachments[0].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pAttachments[1].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pAttachments[2].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pSubpasses[0].colorAttachmentCount == 3);
    assert(info->pSubpasses[0].pDepthStencilAttachment->attachment == 3);
    assert(info->pAttachments[0].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[1].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[0].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[1].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[3].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    assert(info->dependencyCount == 2);
    assert(info->pDependencies[1].dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
    assert(info->pDependencies[1].srcAccessMask & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
    assert(info->pDependencies[1].srcStageMask & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
    *result = reinterpret_cast<VkRenderPass>(uintptr_t(3));
    return VK_SUCCESS;
}

int main()
{
    DeferredEnvironment environment;
    environment.sun_direction[0] = 0.f;
    environment.sun_direction[1] = -4.f;
    environment.sun_direction[2] = 0.f;
    environment.sun_color[0] = 0.3f;
    environment.sun_color[1] = 0.6f;
    environment.sun_color[2] = 0.9f;
    environment.ambient_color[0] = 0.2f;
    environment.ambient_color[1] = 0.2f;
    environment.ambient_color[2] = 0.2f;
    environment.hemi_color[0] = 0.4f;
    environment.hemi_color[1] = 0.4f;
    environment.hemi_color[2] = 0.4f;
    const DeferredLight environment_light = make_environment_deferred_light(environment);
    assert(environment_light.direction_ambient[0] == 0.f);
    assert(environment_light.direction_ambient[1] == -1.f);
    assert(environment_light.direction_ambient[2] == 0.f);
    assert(environment_light.color[0] == 0.3f && environment_light.color[1] == 0.6f &&
        environment_light.color[2] == 0.9f);
    assert(environment_light.direction_ambient[3] > 0.19f && environment_light.direction_ambient[3] < 0.21f);
    assert(environment_light.grade[3] > 0.39f && environment_light.grade[3] < 0.41f);
    environment.sun_direction[0] = environment.sun_direction[1] = environment.sun_direction[2] = 0.f;
    environment.sun_color[0] = -1.f;
    environment.ambient_color[0] = std::numeric_limits<float>::quiet_NaN();
    const DeferredLight safe_light = make_environment_deferred_light(environment);
    assert(safe_light.direction_ambient[1] == -1.f && safe_light.color[0] == 0.f);
    assert(std::isfinite(safe_light.direction_ambient[3]));

    VkRenderPass result{};
    std::string error;
    FrameDispatch dispatch{};
    assert(!create_gbuffer_render_pass(VK_NULL_HANDLE, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D24_UNORM_S8_UINT, dispatch, result, error));
    dispatch.create_render_pass = create_render_pass;
    assert(create_gbuffer_render_pass(reinterpret_cast<VkDevice>(uintptr_t(2)),
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_FORMAT_D24_UNORM_S8_UINT, dispatch, result, error));
    assert(result && error.empty());

    ScenePassDispatch pass_dispatch{};
    pass_dispatch.create_pipeline_layout = create_layout;
    pass_dispatch.destroy_pipeline_layout = destroy_layout;
    pass_dispatch.create_graphics_pipelines = create_pipeline;
    pass_dispatch.destroy_pipeline = destroy_pipeline;
    pass_dispatch.cmd_bind_pipeline = bind_pipeline;
    pass_dispatch.cmd_set_viewport = viewport;
    pass_dispatch.cmd_set_scissor = scissor;
    pass_dispatch.cmd_bind_vertex_buffers = bind_vertices;
    pass_dispatch.cmd_bind_index_buffer = bind_indices;
    pass_dispatch.cmd_push_constants = push_constants;
    pass_dispatch.cmd_draw_indexed = draw_indexed;
    pass_dispatch.create_descriptor_set_layout = create_descriptor_layout;
    pass_dispatch.destroy_descriptor_set_layout = destroy_descriptor_layout;
    pass_dispatch.create_descriptor_pool = create_pool;
    pass_dispatch.destroy_descriptor_pool = destroy_pool;
    pass_dispatch.allocate_descriptor_sets = allocate_sets;
    pass_dispatch.free_descriptor_sets = free_sets;
    pass_dispatch.update_descriptor_sets = update_sets;
    pass_dispatch.cmd_bind_descriptor_sets = bind_sets;
    pass_dispatch.cmd_draw = draw;

    DeferredPass deferred;
    assert(deferred.initialize(handle<VkDevice>(2), handle<VkRenderPass>(10),
        handle<VkRenderPass>(11), handle<VkShaderModule>(12), handle<VkShaderModule>(13),
        handle<VkShaderModule>(14), handle<VkShaderModule>(18), handle<VkShaderModule>(15), handle<VkShaderModule>(16),
        handle<VkShaderModule>(17),
        pass_dispatch, error));
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        handle<VkShaderModule>(50), handle<VkShaderModule>(51), SurfaceMode::Opaque, false, error));
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        handle<VkShaderModule>(50), handle<VkShaderModule>(51), SurfaceMode::Opaque, false, error));
    assert(pipeline_count == 7);
    assert(!deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        handle<VkShaderModule>(50), handle<VkShaderModule>(51), SurfaceMode::AlphaTest, false, error));
    assert(deferred.create_game_pipeline("vk\\object_blended.vs", "vk\\object_blended.ps",
        handle<VkShaderModule>(52), handle<VkShaderModule>(53), SurfaceMode::Transparent, false, error));
    assert(deferred.create_game_pipeline("vk\\skinned_4.vs", "vk\\object_opaque.ps",
        handle<VkShaderModule>(54), handle<VkShaderModule>(55), SurfaceMode::Opaque, false, error, true));
    assert(deferred.has_game_pipeline("vk\\object_blended.vs", "vk\\object_blended.ps"));
    assert(deferred.require_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        SurfaceMode::Opaque, false, false, error));
    assert(!deferred.require_game_pipeline("missing.vs", "missing.ps",
        SurfaceMode::Opaque, false, false, error) && error.find("unknown") != std::string::npos);
    assert(!deferred.require_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        SurfaceMode::AlphaTest, false, false, error) && error.find("parameters") != std::string::npos);
    assert(!deferred.create_game_pipeline("wrong.ps", "wrong.vs",
        handle<VkShaderModule>(60), handle<VkShaderModule>(61), SurfaceMode::Opaque, false, error) &&
        error.find("stages") != std::string::npos);
    VkDescriptorSet material{};
    const auto albedo = handle<VkImageView>(50);
    const auto sampler = handle<VkSampler>(51);
    assert(deferred.material(albedo, sampler, material, error));
    assert(material == updated_material && descriptor_updates == 3 &&
        updated_albedo == albedo && updated_sampler == sampler);
    expected_surface.bump_mode = 3.f;
    expected_surface.detail_normal = 2.f;
    expected_surface.material = 2.f;
    expected_surface.material_weight = .35f;
    deferred.set_surface_parameters(material, expected_surface);

    FrameRecordingContext geometry_frame{handle<VkCommandBuffer>(17),
        handle<VkRenderPass>(10), handle<VkFramebuffer>(18), {640, 480}, 0, 0};
    for (unsigned i = 0; i < 16; ++i) expected_mvp[i] = static_cast<float>(i + 1);
    expected_vertex_base = 7;
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::Opaque,
        0, nullptr, nullptr, 128.f / 255.f, nullptr, 1.f, nullptr, nullptr,
        expected_vertex_base));
    expected_vertex_base = 0;
    assert(bound_pipeline == handle<VkPipeline>(100));
    assert(surface_push);
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, SurfaceMode::Opaque, 0,
        "vk\\level_opaque.vs", "vk\\level_opaque.ps"));
    assert(bound_pipeline == handle<VkPipeline>(106));
    assert(!deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, SurfaceMode::Opaque, 0,
        "missing.vs", "missing.ps"));
    VkDescriptorSet pose{};
    assert(deferred.pose_descriptor(handle<VkBuffer>(23), sizeof(float) * 16, pose, error));
    assert(deferred.record_skinned(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, pose, SurfaceMode::Opaque, false, 0,
        "vk\\skinned_4.vs", "vk\\object_opaque.ps"));
    assert(bound_pipeline == handle<VkPipeline>(108));
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::AlphaTest));
    assert(bound_pipeline == handle<VkPipeline>(101));
    expected_first_index = 3;
    assert(!deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::Transparent, 3));
    assert(draw_count == 4);
    FrameRecordingContext hud_frame{handle<VkCommandBuffer>(17),
        handle<VkRenderPass>(11), handle<VkFramebuffer>(18), {640, 480}, 0, 0};
    assert(deferred.forward_set(0, handle<VkBuffer>(24), error));
    assert(!deferred.record_transparent(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, 3));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, 3));
    assert(bound_pipeline == handle<VkPipeline>(102));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, 3, "vk\\object_blended.vs", "vk\\object_blended.ps"));
    assert(bound_pipeline == handle<VkPipeline>(107));
    assert(deferred.record_hud(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, 3));
    assert(bound_pipeline == handle<VkPipeline>(103));
    assert(draw_count == 7);
    VkDescriptorSet gbuffer{}, weather_set{};
    assert(deferred.gbuffer(albedo, handle<VkImageView>(52), handle<VkImageView>(53),
        handle<VkImageView>(54), sampler, gbuffer, error));
    deferred.bind_sun_shadow(gbuffer, handle<VkImageView>(55), sampler, handle<VkBuffer>(25));
    assert(deferred.weather_set(handle<VkImageView>(60), handle<VkImageView>(61),
        handle<VkImageView>(62), handle<VkImageView>(63), handle<VkImageView>(64),
        handle<VkImageView>(65), sampler, weather_set, error));
    WeatherLighting weather{};
    assert(deferred.record_lighting(hud_frame, gbuffer, environment_light, weather_set, &weather));
    assert(bound_pipeline == handle<VkPipeline>(105) && weather_bind && weather_push);
    deferred.release_gbuffer(gbuffer);
    deferred.release_gbuffer(weather_set);
    deferred.release_gbuffer(material);
    deferred.release_pose_descriptor(pose);
    deferred.release_forward_sets();
    assert(material == VK_NULL_HANDLE && pose == VK_NULL_HANDLE && descriptor_frees == 5);
    deferred.begin_game_pipeline_reload();
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        handle<VkShaderModule>(56), handle<VkShaderModule>(57), SurfaceMode::Opaque, false, error));
    deferred.abort_game_pipeline_reload();
    assert(deferred.has_game_pipeline("vk\\object_blended.vs", "vk\\object_blended.ps"));
    assert(pipeline_destroys == 1);
    deferred.begin_game_pipeline_reload();
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps",
        handle<VkShaderModule>(58), handle<VkShaderModule>(59), SurfaceMode::Opaque, false, error));
    deferred.commit_game_pipeline_reload();
    assert(deferred.has_game_pipeline("vk\\level_opaque.vs", "vk\\level_opaque.ps"));
    assert(!deferred.has_game_pipeline("vk\\object_blended.vs", "vk\\object_blended.ps"));
    assert(pipeline_destroys == 4);
    assert(deferred.create_game_pipeline("vk\\object_cutout.vs", "vk\\object_cutout.ps",
        handle<VkShaderModule>(60), handle<VkShaderModule>(61), SurfaceMode::AlphaTest, false, error));
    assert(deferred.create_game_pipeline("vk\\object_double_sided.vs", "vk\\object_double_sided.ps",
        handle<VkShaderModule>(62), handle<VkShaderModule>(63), SurfaceMode::Opaque, false, error));
    for (unsigned weights = 1; weights <= 3; ++weights)
    {
        const auto vertex = "vk\\skinned_" + std::to_string(weights) + ".vs";
        assert(deferred.create_game_pipeline(vertex, "vk\\object_opaque.ps",
            handle<VkShaderModule>(64 + 2 * (weights - 1)),
            handle<VkShaderModule>(65 + 2 * (weights - 1)),
            SurfaceMode::Opaque, false, error, true));
        assert(deferred.has_game_pipeline(vertex, "vk\\object_opaque.ps"));
    }
    assert(deferred.create_game_pipeline("vk\\hud_skinned_4.vs", "vk\\object_blended.ps", handle<VkShaderModule>(70), handle<VkShaderModule>(71),
                                         SurfaceMode::Opaque, true, error, true));

    uint32_t module_creates = 0, module_destroys = 0, source_reads = 0;
    static uint32_t *creates;
    static uint32_t *destroys;
    creates = &module_creates;
    destroys = &module_destroys;
    GameShaderResources resources;
    deferred.set_game_pipeline_request([&](const std::string &vs, const std::string &ps, SurfaceMode mode, bool hud, bool skinned, std::string &reason) {
        return resources.pipeline(deferred, vs, ps, mode, hud, skinned, reason);
    });
    ShaderModuleDispatch modules{+[](VkDevice, const VkShaderModuleCreateInfo *, const VkAllocationCallbacks *, VkShaderModule *out) {
                                     *out = handle<VkShaderModule>(72 + (*creates)++);
                                     return VK_SUCCESS;
                                 },
                                 +[](VkDevice, VkShaderModule, const VkAllocationCallbacks *) { ++*destroys; }};
    resources.configure(handle<VkDevice>(2), modules, [&](const std::string &name, std::vector<uint8_t> &bytes, std::string &reason) {
        ++source_reads;
        if (name != "legacy\\deffer_base.vs.spv" && name != "legacy\\deffer_base.ps.spv")
        {
            reason = "missing precompiled SPIR-V: " + name;
            return false;
        }
        const uint32_t code[]{0x07230203, 0x00010000, 0, 2, 0, 0x0005000f, name.find(".vs.") != std::string::npos ? 0u : 4u, 1, 0x6e69616d, 0};
        bytes.assign(reinterpret_cast<const uint8_t *>(code), reinterpret_cast<const uint8_t *>(code) + sizeof(code));
        return true;
    });
    GameShaderResource *svs = nullptr;
    assert(resources.shader("legacy/deffer_base", GameShaderStage::Vertex, "main", 0, svs, error));
    assert(svs && svs->stage == GameShaderStage::Vertex && svs->module.handle() == handle<VkShaderModule>(72));
    assert(deferred.request_game_pipeline("legacy/deffer_base", "legacy/deffer_base.ps", SurfaceMode::Opaque, false, false, error));
    assert(pipeline_count == 18 && module_creates == 2 && source_reads == 2);
    assert(resources.pipeline(deferred, "legacy\\deffer_base.vs.spv", "legacy/deffer_base", SurfaceMode::Opaque, false, false, error));
    assert(pipeline_count == 18 && module_creates == 2 && source_reads == 2);
    assert(deferred.require_game_pipeline("legacy\\deffer_base.vs", "legacy\\deffer_base.ps", SurfaceMode::Opaque, false, false, error));
    assert(!deferred.request_game_pipeline("unknown", "legacy/deffer_base", SurfaceMode::Opaque, false, false, error) &&
           error.find("unknown.vs") != std::string::npos);
    assert(!resources.pipeline(deferred, "legacy/deffer_base.ps", "legacy/deffer_base", SurfaceMode::Opaque, false, false, error) &&
           error.find("stage mismatch") != std::string::npos);
    assert(!resources.pipeline(deferred, "legacy/deffer_base", "legacy/deffer_base", SurfaceMode::AlphaTest, false, false, error) &&
           error.find("incompatible") != std::string::npos);
    assert(!resources.shader("legacy/deffer_base", GameShaderStage::Vertex, "main", 1, svs, error));
    assert(!svs && error.find("parameters") != std::string::npos);
    assert(!resources.shader("../escape", GameShaderStage::Vertex, "main", 0, svs, error));
    resources.clear();
    assert(module_destroys == 2 && resources.size() == 0);
    resources.destroy();
    assert(deferred.create_game_pipeline("vk\\level_laemb_base.vs", "vk\\level_laemb_multiply.ps",
        handle<VkShaderModule>(90), handle<VkShaderModule>(91), SurfaceMode::Opaque, false, error));
    assert(deferred.create_game_pipeline("vk\\level_detail_cutout.vs", "vk\\level_detail_cutout.ps",
        handle<VkShaderModule>(94), handle<VkShaderModule>(95), SurfaceMode::AlphaTest, false, error));
    assert(pipeline_count == 20);
    std::array<VkImageView, 7> views{};
    for (size_t i = 0; i < views.size(); ++i) views[i] = handle<VkImageView>(101 + i);
    VkDescriptorSet detail{};
    assert(deferred.terrain_material(views, sampler, detail, error));
    std::array<VkImageView, 13> surface_images{};
    for (size_t i = 0; i < surface_images.size(); ++i)
        surface_images[i] = handle<VkImageView>(201 + i);
    distinct_surface_images = true;
    deferred.update_surface_textures(detail, surface_images, sampler);
    assert(surface_bindings_checked);
    distinct_surface_images = false;
    expected_surface = {};
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, SurfaceMode::AlphaTest, 3,
        "vk\\level_detail_cutout.vs", "vk\\level_detail_cutout.ps", .4f, nullptr, 2.5f));
    assert(cutout_alpha_push && cutout_scale_push);
    assert(deferred.create_game_pipeline("vk\\level_detail_blended.vs", "vk\\level_detail_blended.ps",
        handle<VkShaderModule>(96), handle<VkShaderModule>(97), SurfaceMode::Transparent, false, error));
    assert(deferred.create_game_pipeline("vk\\level_detail_blended.vs", "vk\\level_detail_unlit_blended.ps",
        handle<VkShaderModule>(98), handle<VkShaderModule>(99), SurfaceMode::Transparent, false, error));
    assert(pipeline_count == 22);
    assert(deferred.forward_set(0, handle<VkBuffer>(24), error));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, 3, "vk\\level_detail_blended.vs", "vk\\level_detail_blended.ps",
        nullptr, 0.f, nullptr, 1.75f));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, 3, "vk\\level_detail_blended.vs", "vk\\level_detail_unlit_blended.ps",
        nullptr, 0.f, nullptr, 1.75f));
    assert(forward_detail_scale_push);
    assert(deferred.create_game_pipeline("vk\\level_detail_cutout.vs", "vk\\level_detail_unlit_cutout.ps",
        handle<VkShaderModule>(100), handle<VkShaderModule>(101), SurfaceMode::AlphaTest, false, error));
    assert(pipeline_count == 23);
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, SurfaceMode::AlphaTest, 3,
        "vk\\level_detail_cutout.vs", "vk\\level_detail_unlit_cutout.ps", .4f, nullptr, 2.5f));
    assert(deferred.create_game_pipeline("vk\\object_environment.vs", "vk\\object_environment_opaque.ps",
        handle<VkShaderModule>(102), handle<VkShaderModule>(103), SurfaceMode::Opaque, false, error));
    assert(deferred.create_game_pipeline("vk\\skinned_2.vs", "vk\\skinned_environment_opaque.ps",
        handle<VkShaderModule>(104), handle<VkShaderModule>(105), SurfaceMode::Opaque, false, error, true));
    expected_first_index = 0;
    environment_camera_push = environment_surface_push = false;
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, SurfaceMode::Opaque, 0,
        "vk\\object_environment.vs", "vk\\object_environment_opaque.ps", 0.f, nullptr, 1.f,
        expected_environment_camera));
    assert(environment_camera_push && environment_surface_push);
    environment_camera_push = environment_surface_push = false;
    assert(deferred.pose_descriptor(handle<VkBuffer>(23), sizeof(float) * 16, pose, error));
    assert(deferred.record_skinned(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, pose, SurfaceMode::Opaque, false, 0,
        "vk\\skinned_2.vs", "vk\\skinned_environment_opaque.ps", 0.f, nullptr, 1.f,
        expected_environment_camera));
    assert(environment_camera_push && environment_surface_push);
    for (unsigned blend = 0; blend < 3; ++blend)
    {
        const char* fragment = blend == 0 ? "vk\\world_set_additive.ps" :
            blend == 1 ? "vk\\world_set_multiply.ps" : "vk\\world_set_multiply_2x.ps";
        assert(deferred.create_game_pipeline("vk\\level_opaque.vs", fragment,
            handle<VkShaderModule>(106), handle<VkShaderModule>(107 + blend),
            SurfaceMode::Transparent, false, error));
        assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, 0, "vk\\level_opaque.vs", fragment));
    }
    for (unsigned blend = 0; blend < 4; ++blend)
    {
        const char* fragment = blend == 0 ? "vk\\world_set_set_2r.ps" :
            blend == 1 ? "vk\\world_set_blend_2r.ps" :
            blend == 2 ? "vk\\world_set_blend_4r.ps" : "vk\\world_set_blended.ps";
        assert(deferred.create_game_pipeline("vk\\level_opaque.vs", fragment,
            handle<VkShaderModule>(106), handle<VkShaderModule>(110 + blend),
            SurfaceMode::Transparent, false, error));
        world_alpha_push = false;
        assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, 0, "vk\\level_opaque.vs", fragment, nullptr, .25f));
        assert(world_alpha_push);
    }
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\world_set_set.ps",
        handle<VkShaderModule>(106), handle<VkShaderModule>(114),
        SurfaceMode::Transparent, false, error));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, 0, "vk\\level_opaque.vs", "vk\\world_set_set.ps"));
    assert(deferred.create_game_pipeline("vk\\skinned_2.vs", "vk\\model_set_blended.ps",
        handle<VkShaderModule>(115), handle<VkShaderModule>(116),
        SurfaceMode::Transparent, false, error, true));
    world_alpha_push = false;
    assert(deferred.record_skinned(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, pose, SurfaceMode::Transparent, false, 0,
        "vk\\skinned_2.vs", "vk\\model_set_blended.ps", .25f));
    assert(world_alpha_push);
    const char* model_fragments[]{"set", "blended", "additive", "multiply",
        "multiply_2x", "alpha_add", "wallmark_multiply_2x", "set_2r",
        "blend_2r", "blend_4r"};
    for (unsigned mode = 0; mode < 10; ++mode)
    {
        const std::string fragment = std::string("vk\\model_set_") + model_fragments[mode] + ".ps";
        assert(deferred.create_game_pipeline("vk\\object_blended.vs", fragment,
            handle<VkShaderModule>(106), handle<VkShaderModule>(117 + mode),
            SurfaceMode::Transparent, false, error));
        assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, 0, "vk\\object_blended.vs", fragment.c_str(),
            nullptr, .25f));
    }
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\level_fog.ps",
        handle<VkShaderModule>(106), handle<VkShaderModule>(127),
        SurfaceMode::Transparent, false, error));
    world_alpha_push = false;
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, 0, "vk\\level_opaque.vs", "vk\\level_fog.ps",
        nullptr, .25f));
    assert(world_alpha_push);
    for (unsigned mode = 0; mode < 10; ++mode)
    {
        const std::string fragment = std::string("vk\\hud_model_set_") + model_fragments[mode] + ".ps";
        assert(deferred.create_game_pipeline("vk\\object_blended.vs", fragment,
            handle<VkShaderModule>(106), handle<VkShaderModule>(128 + mode),
            SurfaceMode::Opaque, true, error));
        expected_world_alpha_ref = mode == 7 ? 0.f : .25f;
        world_alpha_push = false;
        assert(deferred.record_hud(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, 0, "vk\\object_blended.vs", fragment.c_str(),
            expected_world_alpha_ref));
        assert(world_alpha_push == (mode == 1 || mode == 5 || mode == 7 || mode == 8 || mode == 9));
    }
    for (unsigned mode = 0; mode < 10; ++mode)
    {
        const std::string fragment = std::string("vk\\hud_model_set_") + model_fragments[mode] + ".ps";
        assert(deferred.create_game_pipeline("vk\\hud_skinned_2.vs", fragment,
            handle<VkShaderModule>(115), handle<VkShaderModule>(138 + mode),
            SurfaceMode::Opaque, true, error, true));
        expected_world_alpha_ref = mode == 7 ? 0.f : .25f;
        world_alpha_push = false;
        assert(deferred.record_skinned(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, pose, SurfaceMode::Opaque, true, 0,
            "vk\\hud_skinned_2.vs", fragment.c_str(), expected_world_alpha_ref));
        assert(world_alpha_push == (mode == 1 || mode == 5 || mode == 7 || mode == 8 || mode == 9));
    }
    const char* screen_particle_fragments[]{"set", "blended", "additive",
        "multiply", "multiply_2x", "alpha_add"};
    for (unsigned mode = 0; mode < 6; ++mode)
    {
        const std::string fragment = std::string("vk\\hud_particle_screen_") +
            screen_particle_fragments[mode] + ".ps";
        assert(deferred.create_game_pipeline("vk\\level_opaque.vs", fragment,
            handle<VkShaderModule>(106), handle<VkShaderModule>(148 + mode),
            SurfaceMode::Opaque, true, error));
        world_alpha_push = false;
        assert(deferred.record_hud(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
            6, expected_mvp, detail, 0, "vk\\level_opaque.vs", fragment.c_str(), .25f));
        assert(world_alpha_push == (mode == 1 || mode == 5));
    }
    LevelPassGraph two_pass;
    assert(build_level_pass_graph({SurfaceMode::Opaque, true, false, false,
        true, true}, two_pass, error) && two_pass.count == 2);
    assert(two_pass.passes[1].previous_color_pass == 0 &&
        two_pass.passes[1].target == LevelPassTarget::GBuffer &&
        two_pass.passes[1].textures == BaseRole);
    assert(deferred.create_game_pipeline(two_pass.passes[0].vertex,
        two_pass.passes[0].fragment, handle<VkShaderModule>(200),
        handle<VkShaderModule>(201), SurfaceMode::Opaque, false, error));
    const auto prior_draws = draw_count;
    for (uint8_t step = 0; step < two_pass.count; ++step)
        assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
            handle<VkBuffer>(21), 6, expected_mvp, detail, SurfaceMode::Opaque, 0,
            two_pass.passes[step].vertex.c_str(), two_pass.passes[step].fragment.c_str(),
            0.f, nullptr, 1.f, step == 0 ? expected_environment_camera : nullptr));
    assert(draw_count == prior_draws + 2 && bound_pipeline == handle<VkPipeline>(118));
    assert(deferred.create_game_pipeline("vk\\level_opaque.vs", "vk\\particle_world_deferred.ps",
        handle<VkShaderModule>(106), handle<VkShaderModule>(202),
        SurfaceMode::AlphaTest, false, error));
    assert(!deferred.record_transparent(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, 0, "vk\\level_opaque.vs", "vk\\particle_world_deferred.ps"));
    expected_world_alpha_ref = 200.f / 255.f;
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, detail, SurfaceMode::AlphaTest, 0,
        "vk\\level_opaque.vs", "vk\\particle_world_deferred.ps", 200.f / 255.f));
    assert(draw_count == prior_draws + 3);
    deferred.release_pose_descriptor(pose);
    deferred.release_gbuffer(detail);
    deferred.release_forward_sets();
}
