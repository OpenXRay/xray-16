#include "src/Layers/xrRenderVK/DeferredPass.h"

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
uint32_t expected_first_index{};
VkPipeline bound_pipeline{};
VkPipelineLayout geometry_layout{}, lighting_layout{};
VkDescriptorSet updated_material{};
VkImageView updated_albedo{};
VkSampler updated_sampler{};
float expected_mvp[16]{};

VkResult VKAPI_PTR create_layout(VkDevice, const VkPipelineLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkPipelineLayout* output)
{
    assert(info->setLayoutCount == 1 && info->pushConstantRangeCount == 1);
    const auto& range = info->pPushConstantRanges[0];
    if (range.stageFlags == VK_SHADER_STAGE_VERTEX_BIT)
    {
        assert(range.size == sizeof(expected_mvp));
        *output = geometry_layout = handle<VkPipelineLayout>(41);
    }
    else
    {
        assert(range.stageFlags == VK_SHADER_STAGE_FRAGMENT_BIT && range.size == sizeof(DeferredLight));
        *output = lighting_layout = handle<VkPipelineLayout>(42);
    }
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_layout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) {}

VkResult VKAPI_PTR create_pipeline(VkDevice, VkPipelineCache, uint32_t count,
    const VkGraphicsPipelineCreateInfo* info, const VkAllocationCallbacks*, VkPipeline* output)
{
    assert(count == 1 && info->stageCount == 2);
    if (pipeline_count < 2)
    {
        assert(info->renderPass == handle<VkRenderPass>(10));
        assert(info->pVertexInputState->vertexAttributeDescriptionCount == 3);
        const auto* attributes = info->pVertexInputState->pVertexAttributeDescriptions;
        assert(attributes[0].format == VK_FORMAT_R32G32B32_SFLOAT &&
            attributes[0].offset == offsetof(LevelVertex, position));
        assert(attributes[1].format == VK_FORMAT_R32G32B32_SFLOAT &&
            attributes[1].offset == offsetof(LevelVertex, normal));
        assert(attributes[2].format == VK_FORMAT_R32G32_SFLOAT &&
            attributes[2].offset == offsetof(LevelVertex, uv));
        assert(info->pColorBlendState->attachmentCount == 2);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE);
        assert(info->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_LESS);
        assert(info->pDepthStencilState->depthWriteEnable == VK_TRUE);
        assert(info->pColorBlendState->pAttachments[0].blendEnable == VK_FALSE);
    }
    else if (pipeline_count == 2 || pipeline_count == 3)
    {
        assert(info->renderPass == handle<VkRenderPass>(11));
        assert(info->pColorBlendState->attachmentCount == 1);
        assert(info->pDepthStencilState->depthTestEnable == VK_TRUE);
        assert(info->pDepthStencilState->depthWriteEnable == (pipeline_count == 3 ? VK_TRUE : VK_FALSE));
        assert(info->pDepthStencilState->depthCompareOp == VK_COMPARE_OP_LESS);
        assert(info->pColorBlendState->pAttachments[0].blendEnable == VK_TRUE);
    }
    else
    {
        assert(pipeline_count == 4 && info->renderPass == handle<VkRenderPass>(11));
        assert(info->pVertexInputState->vertexBindingDescriptionCount == 0);
        assert(info->pColorBlendState->attachmentCount == 1);
    }
    *output = handle<VkPipeline>(100 + pipeline_count++);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*) {}
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
    uint32_t, uint32_t size, const void* data)
{
    assert(layout == geometry_layout && stage == VK_SHADER_STAGE_VERTEX_BIT && size == sizeof(expected_mvp));
    assert(std::memcmp(data, expected_mvp, sizeof(expected_mvp)) == 0);
}
void VKAPI_PTR draw_indexed(VkCommandBuffer, uint32_t count, uint32_t instances,
    uint32_t first_index, int32_t, uint32_t)
{
    assert(count == 6 && instances == 1);
    assert(first_index == expected_first_index);
    assert(bound_pipeline == handle<VkPipeline>(100 + draw_count));
    ++draw_count;
}
void VKAPI_PTR draw(VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t) {}

VkResult VKAPI_PTR create_descriptor_layout(VkDevice, const VkDescriptorSetLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorSetLayout* output)
{
    assert(info->bindingCount == 1 || info->bindingCount == 2);
    assert(info->pBindings[0].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    if (info->bindingCount == 2)
        assert(info->pBindings[1].binding == 1);
    *output = handle<VkDescriptorSetLayout>(info->bindingCount == 1 ? 31 : 32);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_descriptor_layout(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*) {}
VkResult VKAPI_PTR create_pool(VkDevice, const VkDescriptorPoolCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorPool* output)
{
    assert(info->maxSets == 256 && info->pPoolSizes[0].descriptorCount == 512);
    *output = handle<VkDescriptorPool>(33);
    return VK_SUCCESS;
}
void VKAPI_PTR destroy_pool(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*) {}
VkResult VKAPI_PTR allocate_sets(VkDevice, const VkDescriptorSetAllocateInfo* info,
    VkDescriptorSet* output)
{
    assert(info->descriptorPool == handle<VkDescriptorPool>(33));
    *output = updated_material = handle<VkDescriptorSet>(34);
    return VK_SUCCESS;
}
VkResult VKAPI_PTR free_sets(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*)
{ ++descriptor_frees; return VK_SUCCESS; }
void VKAPI_PTR update_sets(VkDevice, uint32_t count, const VkWriteDescriptorSet* writes,
    uint32_t, const VkCopyDescriptorSet*)
{
    assert(count == 1 && writes[0].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    assert(writes[0].pImageInfo->imageLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    updated_albedo = writes[0].pImageInfo->imageView;
    updated_sampler = writes[0].pImageInfo->sampler;
    ++descriptor_updates;
}
void VKAPI_PTR bind_sets(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout layout,
    uint32_t, uint32_t count, const VkDescriptorSet* sets, uint32_t, const uint32_t*)
{
    assert(layout == geometry_layout && count == 1 && *sets == updated_material);
}
}

static VkResult VKAPI_PTR create_render_pass(VkDevice, const VkRenderPassCreateInfo* info,
    const VkAllocationCallbacks*, VkRenderPass* result)
{
    assert(info->attachmentCount == 3 && info->subpassCount == 1);
    assert(info->pAttachments[0].format == VK_FORMAT_R8G8B8A8_UNORM);
    assert(info->pAttachments[1].format == VK_FORMAT_R8G8B8A8_UNORM);
    assert(info->pAttachments[2].format == VK_FORMAT_D24_UNORM_S8_UINT);
    assert(info->pAttachments[0].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pAttachments[1].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pAttachments[2].loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->pSubpasses[0].colorAttachmentCount == 2);
    assert(info->pSubpasses[0].pDepthStencilAttachment->attachment == 2);
    assert(info->pAttachments[0].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[1].finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(info->pAttachments[0].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[1].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].storeOp == VK_ATTACHMENT_STORE_OP_STORE);
    assert(info->pAttachments[2].finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
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
    assert(environment_light.direction_ambient[3] > 0.29f && environment_light.direction_ambient[3] < 0.31f);
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
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_D24_UNORM_S8_UINT, dispatch, result, error));
    dispatch.create_render_pass = create_render_pass;
    assert(create_gbuffer_render_pass(reinterpret_cast<VkDevice>(uintptr_t(2)),
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM,
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
        handle<VkShaderModule>(14), handle<VkShaderModule>(15), handle<VkShaderModule>(16),
        pass_dispatch, error));
    VkDescriptorSet material{};
    const auto albedo = handle<VkImageView>(50);
    const auto sampler = handle<VkSampler>(51);
    assert(deferred.material(albedo, sampler, material, error));
    assert(material == updated_material && descriptor_updates == 1 &&
        updated_albedo == albedo && updated_sampler == sampler);

    FrameRecordingContext geometry_frame{handle<VkCommandBuffer>(17),
        handle<VkRenderPass>(10), handle<VkFramebuffer>(18), {640, 480}, 0, 0};
    for (unsigned i = 0; i < 16; ++i) expected_mvp[i] = static_cast<float>(i + 1);
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::Opaque));
    assert(bound_pipeline == handle<VkPipeline>(100));
    assert(deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::AlphaTest));
    assert(bound_pipeline == handle<VkPipeline>(101));
    expected_first_index = 3;
    assert(!deferred.record_geometry(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, SurfaceMode::Transparent, 3));
    assert(draw_count == 2);
    FrameRecordingContext hud_frame{handle<VkCommandBuffer>(17),
        handle<VkRenderPass>(11), handle<VkFramebuffer>(18), {640, 480}, 0, 0};
    assert(!deferred.record_transparent(geometry_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, 3));
    assert(deferred.record_transparent(hud_frame, handle<VkBuffer>(20),
        handle<VkBuffer>(21), 6, expected_mvp, material, 3));
    assert(bound_pipeline == handle<VkPipeline>(102));
    assert(deferred.record_hud(hud_frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        6, expected_mvp, material, 3));
    assert(bound_pipeline == handle<VkPipeline>(103));
    assert(draw_count == 4 && error.empty());
    deferred.release_gbuffer(material);
    assert(material == VK_NULL_HANDLE && descriptor_frees == 1);
}
