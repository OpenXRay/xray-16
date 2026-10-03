#include "src/Layers/xrRenderVK/ImageStateTracker.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
uint32_t barrier_calls = 0;
VkPipelineStageFlags source_stage{};
VkPipelineStageFlags destination_stage{};
std::vector<VkImageMemoryBarrier> last_barriers;

void VKAPI_CALL capture_barrier(VkCommandBuffer, VkPipelineStageFlags source, VkPipelineStageFlags destination,
    VkDependencyFlags, uint32_t, const VkMemoryBarrier*, uint32_t, const VkBufferMemoryBarrier*,
    uint32_t image_count, const VkImageMemoryBarrier* images)
{
    ++barrier_calls;
    source_stage = source;
    destination_stage = destination;
    last_barriers.assign(images, images + image_count);
}
}

int main()
{
    using namespace xray::render::vulkan;
    const auto fake_image = reinterpret_cast<VkImage>(static_cast<uintptr_t>(1));
    const auto fake_command = reinterpret_cast<VkCommandBuffer>(static_cast<uintptr_t>(2));
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 4;
    range.layerCount = 6;

    ImageStateTracker states;
    ImageStateDispatch dispatch{capture_barrier};
    std::string error;
    assert(states.register_image(fake_image, range, ImageUse::Undefined, error));
    assert(!states.register_image(fake_image, range, ImageUse::Undefined, error));
    assert(states.transition(fake_command, fake_image, ImageUse::TransferDestination, dispatch, error));
    assert(barrier_calls == 1);
    assert(source_stage == VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    assert(destination_stage == VK_PIPELINE_STAGE_TRANSFER_BIT);
    assert(last_barriers.size() == 1);
    assert(last_barriers[0].oldLayout == VK_IMAGE_LAYOUT_UNDEFINED);
    assert(last_barriers[0].newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    assert(last_barriers[0].dstAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT);
    assert(last_barriers[0].subresourceRange.levelCount == 4);
    assert(last_barriers[0].subresourceRange.layerCount == 6);

    assert(states.transition(fake_command, fake_image, ImageUse::TransferDestination, dispatch, error));
    assert(barrier_calls == 1);
    assert(states.transition(fake_command, fake_image, ImageUse::Sampled, dispatch, error));
    assert(barrier_calls == 2);
    assert(source_stage == VK_PIPELINE_STAGE_TRANSFER_BIT);
    assert(destination_stage == (VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT));
    assert(last_barriers.size() == 1);
    assert(last_barriers[0].oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    assert(last_barriers[0].newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    assert(last_barriers[0].srcAccessMask == VK_ACCESS_TRANSFER_WRITE_BIT);
    assert(last_barriers[0].dstAccessMask == VK_ACCESS_SHADER_READ_BIT);
    assert(!states.transition(fake_command, fake_image, ImageUse::Undefined, dispatch, error));
    assert(barrier_calls == 2);

    const auto layered_image = reinterpret_cast<VkImage>(static_cast<uintptr_t>(4));
    assert(states.register_image(layered_image, range, ImageUse::Undefined, error));
    VkImageSubresourceRange partial_range{};
    partial_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    partial_range.baseMipLevel = 1;
    partial_range.levelCount = 1;
    partial_range.baseArrayLayer = 2;
    partial_range.layerCount = 2;
    assert(states.transition(fake_command, layered_image, partial_range, ImageUse::TransferDestination,
        dispatch, error));
    assert(last_barriers.size() == 1);
    assert(last_barriers[0].subresourceRange.baseMipLevel == 1);
    assert(last_barriers[0].subresourceRange.levelCount == 1);
    assert(last_barriers[0].subresourceRange.baseArrayLayer == 2);
    assert(last_barriers[0].subresourceRange.layerCount == 2);

    assert(states.transition(fake_command, layered_image, ImageUse::Sampled, dispatch, error));
    assert(last_barriers.size() == 5);
    assert(source_stage == (VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT));
    bool found_transfer_range = false;
    for (const VkImageMemoryBarrier& barrier : last_barriers)
    {
        const VkImageSubresourceRange& transitioned = barrier.subresourceRange;
        if (barrier.oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            found_transfer_range = true;
            assert(transitioned.baseMipLevel == 1);
            assert(transitioned.levelCount == 1);
            assert(transitioned.baseArrayLayer == 2);
            assert(transitioned.layerCount == 2);
        }
    }
    assert(found_transfer_range);
    const uint32_t calls_after_layered_transition = barrier_calls;
    assert(states.transition(fake_command, layered_image, partial_range, ImageUse::Sampled, dispatch, error));
    assert(barrier_calls == calls_after_layered_transition);

    partial_range.baseArrayLayer = 5;
    partial_range.layerCount = 2;
    assert(!states.transition(fake_command, layered_image, partial_range, ImageUse::ColorAttachment,
        dispatch, error));
    assert(barrier_calls == calls_after_layered_transition);

    const auto depth_stencil_image = reinterpret_cast<VkImage>(static_cast<uintptr_t>(5));
    VkImageSubresourceRange depth_stencil_range{};
    depth_stencil_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depth_stencil_range.levelCount = 1;
    depth_stencil_range.layerCount = 1;
    assert(states.register_image(depth_stencil_image, depth_stencil_range, ImageUse::Undefined, error));
    VkImageSubresourceRange depth_only_range = depth_stencil_range;
    depth_only_range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    assert(states.transition(fake_command, depth_stencil_image, depth_only_range,
        ImageUse::DepthStencilAttachment, dispatch, error));
    assert(last_barriers.size() == 1);
    assert(last_barriers[0].subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT);
    assert(states.transition(fake_command, depth_stencil_image, ImageUse::Sampled, dispatch, error));
    assert(last_barriers.size() == 2);
    bool found_depth_transition = false;
    bool found_stencil_transition = false;
    for (const VkImageMemoryBarrier& barrier : last_barriers)
    {
        if (barrier.subresourceRange.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
        {
            found_depth_transition = true;
            assert(barrier.oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        }
        else if (barrier.subresourceRange.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
        {
            found_stencil_transition = true;
            assert(barrier.oldLayout == VK_IMAGE_LAYOUT_UNDEFINED);
        }
    }
    assert(found_depth_transition && found_stencil_transition);

    const auto unknown_image = reinterpret_cast<VkImage>(static_cast<uintptr_t>(3));
    assert(!states.transition(fake_command, unknown_image, ImageUse::Sampled, dispatch, error));
    assert(states.forget_image(fake_image));
    assert(!states.forget_image(fake_image));
}
