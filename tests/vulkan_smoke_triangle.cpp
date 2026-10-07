#include "src/Layers/xrRenderVK/SmokeTrianglePass.h"
#include "src/Layers/xrRenderVK/SmokeShaders.h"

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace
{
template <typename T> T handle(uintptr_t value)
{
    if constexpr (std::is_pointer_v<T>) return reinterpret_cast<T>(value);
    else return static_cast<T>(value);
}
uint32_t draws = 0;
uint32_t destroyed = 0;
VkResult pipeline_result = VK_SUCCESS;

VkResult VKAPI_CALL create_layout(VkDevice, const VkPipelineLayoutCreateInfo* info,
    const VkAllocationCallbacks*, VkPipelineLayout* layout)
{
    assert(info->setLayoutCount == 0);
    *layout = handle<VkPipelineLayout>(1);
    return VK_SUCCESS;
}
void VKAPI_CALL destroy_layout(VkDevice, VkPipelineLayout, const VkAllocationCallbacks*) { ++destroyed; }
VkResult VKAPI_CALL create_pipeline(VkDevice, VkPipelineCache, uint32_t count,
    const VkGraphicsPipelineCreateInfo* info, const VkAllocationCallbacks*, VkPipeline* pipeline)
{
    assert(count == 1 && info->stageCount == 2);
    assert(info->pStages[0].stage == VK_SHADER_STAGE_VERTEX_BIT);
    assert(info->pStages[1].stage == VK_SHADER_STAGE_FRAGMENT_BIT);
    assert(info->pVertexInputState->vertexBindingDescriptionCount == 0);
    assert(info->pColorBlendState->attachmentCount == 1);
    if (pipeline_result == VK_SUCCESS) *pipeline = handle<VkPipeline>(2);
    return pipeline_result;
}
void VKAPI_CALL destroy_pipeline(VkDevice, VkPipeline, const VkAllocationCallbacks*) { ++destroyed; }
void VKAPI_CALL bind_pipeline(VkCommandBuffer, VkPipelineBindPoint, VkPipeline pipeline)
{
    assert(pipeline == handle<VkPipeline>(2));
}
void VKAPI_CALL viewport(VkCommandBuffer, uint32_t, uint32_t, const VkViewport* value)
{
    assert(value->width == 960 && value->height == 540);
}
void VKAPI_CALL scissor(VkCommandBuffer, uint32_t, uint32_t, const VkRect2D* value)
{
    assert(value->extent.width == 960 && value->extent.height == 540);
}
void VKAPI_CALL draw(VkCommandBuffer, uint32_t vertices, uint32_t instances, uint32_t, uint32_t)
{
    assert(vertices == 3 && instances == 1);
    ++draws;
}
}

int main()
{
    using namespace xray::render::vulkan;
    static_assert(sizeof(smoke::TriangleVertex) >= 20);
    static_assert(sizeof(smoke::TriangleFragment) >= 20);
    assert(smoke::TriangleVertex[0] == 0x07230203);
    assert(smoke::TriangleFragment[0] == 0x07230203);
    SmokeTriangleDispatch dispatch{create_layout, destroy_layout, create_pipeline, destroy_pipeline,
        bind_pipeline, viewport, scissor, draw};
    SmokeTrianglePass pass;
    std::string error;
    const auto device = handle<VkDevice>(4);
    const auto render_pass = handle<VkRenderPass>(5);
    assert(pass.initialize(device, render_pass, handle<VkShaderModule>(6),
        handle<VkShaderModule>(7), dispatch, error));
    const FrameRecordingContext frame{handle<VkCommandBuffer>(8), render_pass,
        handle<VkFramebuffer>(9), {960, 540}, 0, 0};
    pass.record(frame);
    assert(draws == 1);
    auto wrong_pass = frame;
    wrong_pass.render_pass = handle<VkRenderPass>(10);
    pass.record(wrong_pass);
    assert(draws == 1);
    pass.destroy();
    assert(destroyed == 2);
    pipeline_result = VK_ERROR_INITIALIZATION_FAILED;
    assert(!pass.initialize(device, render_pass, handle<VkShaderModule>(6),
        handle<VkShaderModule>(7), dispatch, error));
    assert(destroyed == 3);
}
