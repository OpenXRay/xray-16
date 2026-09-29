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
uint32_t scissor_width = 0;
VkResult ui_result = VK_SUCCESS;
VkPipeline bound = VK_NULL_HANDLE;

VkResult VKAPI_CALL create_layout(VkDevice, const VkPipelineLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkPipelineLayout* layout)
{
    assert(info->pushConstantRangeCount == 1);
    assert(info->pPushConstantRanges[0].size == (layouts % 2 ? 8u : 96u));
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
{ assert(data && size == (bound == handle<VkPipeline>(11) ? 96u : 8u)); }
void VKAPI_CALL draw(VkCommandBuffer, uint32_t count, uint32_t instances, uint32_t, int32_t, uint32_t)
{
    assert(instances == 1);
    if (bound == handle<VkPipeline>(11)) { assert(count == 3); ++geometry_draws; }
    else { assert(bound == handle<VkPipeline>(12) && count == 6); ++ui_draws; }
}
}

int main()
{
    using namespace xray::render::vulkan;
    assert(scene_shaders::SceneVertex[0] == 0x07230203);
    assert(scene_shaders::UiFragment[0] == 0x07230203);
    ScenePassDispatch dispatch{create_layout, destroy_layout, create_pipeline, destroy_pipeline,
        bind_pipeline, viewport, scissor, bind_vertices, bind_indices, push, draw};
    ScenePass pass;
    std::string error;
    const auto device = handle<VkDevice>(1);
    const auto render_pass = handle<VkRenderPass>(2);
    assert(pass.initialize(device, render_pass, handle<VkShaderModule>(3), handle<VkShaderModule>(4),
        handle<VkShaderModule>(5), handle<VkShaderModule>(6), dispatch, error));
    const FrameRecordingContext frame{handle<VkCommandBuffer>(7), render_pass,
        handle<VkFramebuffer>(8), {960, 540}, 0, 0};
    SceneConstants constants{};
    assert(pass.record_geometry(frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 3, constants));
    const VkRect2D clipped{{-20, 40}, {120, 20}};
    assert(pass.record_ui(frame, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 6, &clipped));
    assert(geometry_draws == 1 && ui_draws == 1 && scissor_width == 100);
    auto wrong = frame;
    wrong.render_pass = handle<VkRenderPass>(9);
    assert(!pass.record_geometry(wrong, handle<VkBuffer>(20), handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 3, constants));
    assert(!pass.record_ui(frame, VK_NULL_HANDLE, handle<VkBuffer>(21),
        VK_INDEX_TYPE_UINT16, 6));
    pass.destroy();
    assert(destroyed_pipelines == 2 && destroyed_layouts == 2);
    ui_result = VK_ERROR_INITIALIZATION_FAILED;
    assert(!pass.initialize(device, render_pass, handle<VkShaderModule>(3), handle<VkShaderModule>(4),
        handle<VkShaderModule>(5), handle<VkShaderModule>(6), dispatch, error));
    assert(destroyed_pipelines == 3 && destroyed_layouts == 4);
}
