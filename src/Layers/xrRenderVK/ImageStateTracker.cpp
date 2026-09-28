#include "ImageStateTracker.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace xray::render::vulkan
{
namespace
{
struct ImageUseInfo
{
    VkImageLayout layout;
    VkAccessFlags access;
    VkPipelineStageFlags stage;
};

struct ImageBarrier
{
    VkImageMemoryBarrier barrier{};
    ImageUse previous_use = ImageUse::Undefined;
};

ImageUseInfo describe(ImageUse use)
{
    switch (use)
    {
    case ImageUse::Undefined:
        return {VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
    case ImageUse::TransferSource:
        return {VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
    case ImageUse::TransferDestination:
        return {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT};
    case ImageUse::Sampled:
        return {VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    case ImageUse::ColorAttachment:
        return {VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    case ImageUse::DepthStencilAttachment:
        return {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT};
    case ImageUse::Present:
        return {VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT};
    }
    return {VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};
}

bool valid_use(ImageUse use)
{
    switch (use)
    {
    case ImageUse::Undefined:
    case ImageUse::TransferSource:
    case ImageUse::TransferDestination:
    case ImageUse::Sampled:
    case ImageUse::ColorAttachment:
    case ImageUse::DepthStencilAttachment:
    case ImageUse::Present:
        return true;
    }
    return false;
}

bool valid_range(const VkImageSubresourceRange& range)
{
    const uint32_t remaining = std::numeric_limits<uint32_t>::max();
    return range.aspectMask != 0 && range.levelCount != 0 && range.layerCount != 0 &&
        range.levelCount != remaining && range.layerCount != remaining &&
        range.baseMipLevel <= remaining - range.levelCount && range.baseArrayLayer <= remaining - range.layerCount;
}

bool contains_range(const VkImageSubresourceRange& outer, const VkImageSubresourceRange& inner)
{
    if (!valid_range(inner) || (inner.aspectMask & outer.aspectMask) != inner.aspectMask ||
        inner.baseMipLevel < outer.baseMipLevel || inner.baseArrayLayer < outer.baseArrayLayer)
        return false;

    const uint32_t mip_offset = inner.baseMipLevel - outer.baseMipLevel;
    const uint32_t layer_offset = inner.baseArrayLayer - outer.baseArrayLayer;
    return mip_offset <= outer.levelCount && inner.levelCount <= outer.levelCount - mip_offset &&
        layer_offset <= outer.layerCount && inner.layerCount <= outer.layerCount - layer_offset;
}

size_t state_index(size_t aspect_index, uint32_t mip_level, uint32_t array_layer,
    const VkImageSubresourceRange& image_range)
{
    const size_t mip_count = image_range.levelCount;
    const size_t layer_count = image_range.layerCount;
    const size_t mip_offset = mip_level - image_range.baseMipLevel;
    const size_t layer_offset = array_layer - image_range.baseArrayLayer;
    return (aspect_index * mip_count + mip_offset) * layer_count + layer_offset;
}
}

bool ImageStateTracker::register_image(VkImage image, const VkImageSubresourceRange& range,
    ImageUse initial_use, std::string& error)
{
    if (image == VK_NULL_HANDLE || !valid_range(range) || !valid_use(initial_use))
    {
        error = "Vulkan image state registration has an invalid image range or state";
        return false;
    }
    if (std::any_of(m_images.begin(), m_images.end(), [image](const ImageState& state)
        { return state.image == image; }))
    {
        error = "Vulkan image is already registered with the state tracker";
        return false;
    }

    ImageState state;
    state.image = image;
    state.range = range;
    try
    {
        VkImageAspectFlags remaining_aspects = range.aspectMask;
        while (remaining_aspects)
        {
            const VkImageAspectFlags aspect = remaining_aspects & (0u - remaining_aspects);
            state.aspects.push_back(static_cast<VkImageAspectFlagBits>(aspect));
            remaining_aspects &= ~aspect;
        }

        const size_t mip_count = range.levelCount;
        const size_t layer_count = range.layerCount;
        if (mip_count > std::numeric_limits<size_t>::max() / layer_count ||
            mip_count * layer_count > std::numeric_limits<size_t>::max() / state.aspects.size())
        {
            error = "Vulkan image subresource range is too large to track";
            return false;
        }
        state.uses.resize(mip_count * layer_count * state.aspects.size(), initial_use);
        m_images.push_back(std::move(state));
    }
    catch (...)
    {
        error = "Vulkan image state tracking allocation failed";
        return false;
    }
    error.clear();
    return true;
}

bool ImageStateTracker::transition(VkCommandBuffer command, VkImage image, ImageUse next_use,
    const ImageStateDispatch& vk, std::string& error)
{
    return transition_range(command, image, nullptr, next_use, vk, error);
}

bool ImageStateTracker::transition(VkCommandBuffer command, VkImage image,
    const VkImageSubresourceRange& range, ImageUse next_use, const ImageStateDispatch& vk, std::string& error)
{
    return transition_range(command, image, &range, next_use, vk, error);
}

bool ImageStateTracker::transition_range(VkCommandBuffer command, VkImage image,
    const VkImageSubresourceRange* requested_range, ImageUse next_use, const ImageStateDispatch& vk,
    std::string& error)
{
    if (command == VK_NULL_HANDLE || !vk.cmd_pipeline_barrier)
    {
        error = "Vulkan image transition procedures are unavailable";
        return false;
    }
    if (!valid_use(next_use) || next_use == ImageUse::Undefined)
    {
        error = "Vulkan image transitions cannot target an invalid or undefined state";
        return false;
    }

    const auto found = std::find_if(m_images.begin(), m_images.end(), [image](const ImageState& state)
        { return state.image == image; });
    if (found == m_images.end())
    {
        error = "Vulkan image is not registered with the state tracker";
        return false;
    }

    const VkImageSubresourceRange& range = requested_range ? *requested_range : found->range;
    if (!contains_range(found->range, range))
    {
        error = "Vulkan image transition range is outside the registered subresources";
        return false;
    }

    try
    {
        std::vector<ImageBarrier> barriers;
        std::vector<size_t> previous_row;
        std::vector<size_t> current_row;
        VkPipelineStageFlags source_stages = 0;
        const ImageUseInfo after = describe(next_use);
        bool has_transition = false;

        for (size_t aspect_index = 0; aspect_index < found->aspects.size(); ++aspect_index)
        {
            const VkImageAspectFlags aspect = found->aspects[aspect_index];
            if (!(range.aspectMask & aspect))
                continue;

            previous_row.clear();
            for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.levelCount; ++mip)
            {
                current_row.clear();
                const uint32_t layer_end = range.baseArrayLayer + range.layerCount;
                uint32_t layer = range.baseArrayLayer;
                while (layer < layer_end)
                {
                    const ImageUse previous_use = found->uses[state_index(aspect_index, mip, layer, found->range)];
                    if (previous_use == next_use)
                    {
                        ++layer;
                        continue;
                    }

                    const uint32_t first_layer = layer++;
                    while (layer < layer_end &&
                        found->uses[state_index(aspect_index, mip, layer, found->range)] == previous_use)
                        ++layer;
                    const uint32_t run_layer_count = layer - first_layer;
                    has_transition = true;
                    source_stages |= describe(previous_use).stage;

                    size_t barrier_index = barriers.size();
                    for (size_t previous_index : previous_row)
                    {
                        ImageBarrier& candidate = barriers[previous_index];
                        VkImageSubresourceRange& candidate_range = candidate.barrier.subresourceRange;
                        if (candidate.previous_use == previous_use && candidate_range.aspectMask == aspect &&
                            candidate_range.baseArrayLayer == first_layer &&
                            candidate_range.layerCount == run_layer_count &&
                            candidate_range.baseMipLevel + candidate_range.levelCount == mip)
                        {
                            barrier_index = previous_index;
                            ++candidate_range.levelCount;
                            break;
                        }
                    }

                    if (barrier_index == barriers.size())
                    {
                        const ImageUseInfo before = describe(previous_use);
                        ImageBarrier entry;
                        entry.previous_use = previous_use;
                        entry.barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        entry.barrier.srcAccessMask = before.access;
                        entry.barrier.dstAccessMask = after.access;
                        entry.barrier.oldLayout = before.layout;
                        entry.barrier.newLayout = after.layout;
                        entry.barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        entry.barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        entry.barrier.image = image;
                        entry.barrier.subresourceRange = {aspect, mip, 1, first_layer, run_layer_count};
                        barriers.push_back(entry);
                        barrier_index = barriers.size() - 1;
                    }
                    current_row.push_back(barrier_index);
                }
                previous_row.swap(current_row);
            }
        }

        if (!has_transition)
        {
            error.clear();
            return true;
        }
        if (barriers.size() > std::numeric_limits<uint32_t>::max())
        {
            error = "Vulkan image transition generated too many barriers";
            return false;
        }

        std::vector<VkImageMemoryBarrier> vk_barriers;
        vk_barriers.reserve(barriers.size());
        for (const ImageBarrier& barrier : barriers)
            vk_barriers.push_back(barrier.barrier);
        vk.cmd_pipeline_barrier(command, source_stages, after.stage, 0, 0, nullptr, 0, nullptr,
            static_cast<uint32_t>(vk_barriers.size()), vk_barriers.data());

        for (size_t aspect_index = 0; aspect_index < found->aspects.size(); ++aspect_index)
        {
            const VkImageAspectFlags aspect = found->aspects[aspect_index];
            if (!(range.aspectMask & aspect))
                continue;
            for (uint32_t mip = range.baseMipLevel; mip < range.baseMipLevel + range.levelCount; ++mip)
                for (uint32_t layer = range.baseArrayLayer; layer < range.baseArrayLayer + range.layerCount; ++layer)
                    found->uses[state_index(aspect_index, mip, layer, found->range)] = next_use;
        }
    }
    catch (...)
    {
        error = "Vulkan image transition allocation failed";
        return false;
    }

    error.clear();
    return true;
}

bool ImageStateTracker::forget_image(VkImage image)
{
    const auto found = std::find_if(m_images.begin(), m_images.end(), [image](const ImageState& state)
        { return state.image == image; });
    if (found == m_images.end())
        return false;
    m_images.erase(found);
    return true;
}
}
