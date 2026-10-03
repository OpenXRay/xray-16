#include "src/Layers/xrRenderVK/ScenePass.h"
#include "src/Layers/xrRenderVK/SceneShaders.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace
{
template <typename T> T handle(uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
uint32_t layouts = 0, pipelines = 0, destroyed_layouts = 0, destroyed_pipelines = 0;
uint32_t geometry_draws = 0, ui_draws = 0;
uint32_t descriptor_updates = 0, descriptor_binds = 0, descriptor_frees = 0;
uint32_t scissor_width = 0;
VkResult ui_result = VK_SUCCESS;
VkPipeline bound = VK_NULL_HANDLE;

VkResult VKAPI_CALL create_layout(VkDevice, const VkPipelineLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkPipelineLayout* layout)
{
    assert(info->pushConstantRangeCount == 1);
    assert(info->pPushConstantRanges[0].size == (layouts % 2 ? 12u : 96u));
    *layout = handle<VkPipelineLayout>(++layouts);
    return VK_SUCCESS;
}
void VKAPI_CALL destroy_layout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) { ++destroyed_layouts; }
VkResult VKAPI_CALL create_pipeline(VkDevice, VkPipelineCache, uint32_t,
    const VkGraphicsPipelineCreateInfo* info, const VkAllocationCallbacks*, VkPipeline* pipeline)
{
    const bool ui = pipelines % 2;
    ++pipelines;
    assert(info->stageCount == 2 && info->pVertexInputState->vertexAttributeDescriptionCount == 3);
    assert(info->pVertexInputState->pVertexBindingDescriptions[0].stride ==
        (ui ? sizeof(xray::render::vulkan::UiVertex) : sizeof(xray::render::vulkan::SceneVertex)));
    assert(info->pColorBlendState->pAttachments[0].blendEnable == (ui ? VK_TRUE : VK_FALSE));
    if (info->pDepthStencilState)
        assert(info->pDepthStencilState->depthTestEnable == (ui ? VK_FALSE : VK_TRUE));
    assert(info->pVertexInputState->pVertexAttributeDescriptions[2].format ==
        (ui ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R32G32B32A32_SFLOAT));
    if (ui && ui_result != VK_SUCCESS) return ui_result;
    *pipeline = handle<VkPipeline>(ui ? 12 : 11);
    return VK_SUCCESS;
}
void VKAPI_CALL destroy_pipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*) { ++destroyed_pipelines; }
void VKAPI_CALL bind_pipeline(VkCommandBuffer, VkPipelineBindPoint, VkPipeline pipeline) { bound = pipeline; }
void VKAPI_CALL viewport(VkCommandBuffer, uint32_t, uint32_t, const VkViewport* v)
{ assert(v->width == 960 && v->height == 540); }
void VKAPI_CALL scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D* rect)
{ scissor_width = rect->extent.width; }
void VKAPI_CALL bind_vertices(VkCommandBuffer, uint32_t, uint32_t, const VkBuffer* buffer, const VkDeviceSize*)
{ assert(*buffer == handle<VkBuffer>(20)); }
void VKAPI_CALL bind_indices(VkCommandBuffer, VkBuffer buffer, VkDeviceSize, VkIndexType type)
{ assert(buffer == handle<VkBuffer>(21) && type == VK_INDEX_TYPE_UINT16); }
void VKAPI_CALL push(VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t,
    uint32_t size, const void* data)
{ assert(data && size == (bound == handle<VkPipeline>(11) ? 96u : 12u)); }
void VKAPI_CALL draw(VkCommandBuffer, uint32_t count, uint32_t instances, uint32_t, int32_t, uint32_t)
{
    assert(instances == 1);
    if (bound == handle<VkPipeline>(11)) { assert(count == 3); ++geometry_draws; }
    else { assert(bound == handle<VkPipeline>(12) && count == 6); ++ui_draws; }
}
VkResult VKAPI_CALL create_descriptor_layout(VkDevice, const VkDescriptorSetLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorSetLayout* layout)
{
    assert(info->bindingCount == 1 && info->pBindings[0].descriptorType ==
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    *layout = handle<VkDescriptorSetLayout>(30);
    return VK_SUCCESS;
}
void VKAPI_CALL destroy_descriptor_layout(VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks*) {}
VkResult VKAPI_CALL create_pool(VkDevice, const VkDescriptorPoolCreateInfo* info,
    const VkAllocationCallbacks*, VkDescriptorPool* pool)
{
    assert(info->maxSets == 128);
    assert(info->flags & VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
    *pool = handle<VkDescriptorPool>(31);
    return VK_SUCCESS;
}
void VKAPI_CALL destroy_pool(VkDevice, VkDescriptorPool, const VkAllocationCallbacks*) {}
VkResult VKAPI_CALL allocate_sets(VkDevice, const VkDescriptorSetAllocateInfo* info,
    VkDescriptorSet* result)
{
    assert(info->pSetLayouts[0] == handle<VkDescriptorSetLayout>(30));
    *result = handle<VkDescriptorSet>(32);
    return VK_SUCCESS;
}
void VKAPI_CALL update_sets(VkDevice, uint32_t count, const VkWriteDescriptorSet* writes,
    uint32_t, const VkCopyDescriptorSet*)
{
    assert(count == 1 && writes[0].pImageInfo->imageView == handle<VkImageView>(33));
    assert(writes[0].pImageInfo->sampler == handle<VkSampler>(34));
    ++descriptor_updates;
}
VkResult VKAPI_CALL free_sets(VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet*)
{
    ++descriptor_frees;
    return VK_SUCCESS;
}
void VKAPI_CALL bind_sets(VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t,
    uint32_t count, const VkDescriptorSet* sets, uint32_t, const uint32_t*)
{
    assert(count == 1 && *sets == handle<VkDescriptorSet>(32));
    ++descriptor_binds;
}
}

int main()
{
    using namespace xray::render::vulkan;
    assert(scene_shaders::SceneVertex[0] == 0x07230203);
    assert(scene_shaders::UiFragment[0] == 0x07230203);
    ScenePassDispatch dispatch{create_layout, destroy_layout, create_pipeline, destroy_pipeline,
        bind_pipeline, viewport, scissor, bind_vertices, bind_indices, push, draw,
        create_descriptor_layout, destroy_descriptor_layout, create_pool, destroy_pool,
        allocate_sets, free_sets, update_sets, bind_sets};
    ScenePass pass;
    std::string error;
    const auto device = handle<VkDevice>(1);
    const auto render_pass = handle<VkRenderPass>(2);
    assert(pass.initialize(device, render_pass, handle<VkShaderModule>(3), handle<VkShaderModule>(4),
        handle<VkShaderModule>(5), handle<VkShaderModule>(6), dispatch, error, true));
    VkDescriptorSet texture = VK_NULL_HANDLE;
    assert(pass.create_ui_texture_set(handle<VkImageView>(33), handle<VkSampler>(34), texture, error));
    assert(descriptor_updates == 1);
    const FrameRecordingContext frame{handle<VkCommandBuffer>(7), render_pass,
        handle<VkFramebuffer>(8), {960, 540}, 0, 0};
    SceneConstants constants{};
    assert(pass.record_geometry(frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 3, constants));
    const VkRect2D clipped{{-20, 40}, {120, 20}};
    assert(pass.record_ui(frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 6, texture, &clipped));
    assert(geometry_draws == 1 && ui_draws == 1 && scissor_width == 100 && descriptor_binds == 1);
    auto wrong = frame;
    wrong.render_pass = handle<VkRenderPass>(9);
    assert(!pass.record_geometry(wrong, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 3, constants));
    assert(!pass.record_ui(frame, VK_NULL_HANDLE, handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 6, texture));
    pass.release_ui_texture_set(texture);
    assert(texture == VK_NULL_HANDLE && descriptor_frees == 1);
    assert(pass.create_ui_texture_set(handle<VkImageView>(33), handle<VkSampler>(34), texture, error));
    pass.release_ui_texture_set(texture);
    assert(texture == VK_NULL_HANDLE && descriptor_frees == 2 && descriptor_updates == 2);
    pass.destroy();
    assert(destroyed_pipelines == 2 && destroyed_layouts == 2);
    ui_result = VK_ERROR_INITIALIZATION_FAILED;
    assert(!pass.initialize(device, render_pass, handle<VkShaderModule>(3), handle<VkShaderModule>(4),
        handle<VkShaderModule>(5), handle<VkShaderModule>(6), dispatch, error));
    assert(destroyed_pipelines == 3 && destroyed_layouts == 4);
}
