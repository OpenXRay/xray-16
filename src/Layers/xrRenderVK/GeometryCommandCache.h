#pragma once

#include "ScenePass.h"
#include <array>
#include <algorithm>
#include <cstring>

namespace xray::render::vulkan
{
// Opt-in scope for consecutive level geometry draws. Any other recorder ends
// the scope, so state set by a different pipeline is never assumed to survive.
class GeometryCommandCache
{
public:
    void end() { enabled_ = false; }
    class Batch
    {
    public:
        explicit Batch(GeometryCommandCache& cache) : cache_(cache) { cache_ = {}; cache_.enabled_ = true; }
        ~Batch() { cache_.end(); }
        Batch(const Batch&) = delete;
        Batch& operator=(const Batch&) = delete;
    private:
        GeometryCommandCache& cache_;
    };

    void bind(const ScenePassDispatch& vk, const FrameRecordingContext& frame,
        VkPipeline pipeline, VkPipelineLayout layout, VkBuffer vertices,
        VkBuffer indices, VkDescriptorSet material)
    {
        if (enabled_ && (command_ != frame.command_buffer || layout_ != layout))
        {
            *this = {};
            enabled_ = true;
        }
        const bool fresh = !enabled_ || !command_ || extent_.width != frame.extent.width ||
            extent_.height != frame.extent.height;
        if (fresh || pipeline_ != pipeline)
            vk.cmd_bind_pipeline(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        if (fresh)
        {
            const VkViewport viewport{0, 0, float(frame.extent.width), float(frame.extent.height), 0, 1};
            const VkRect2D scissor{{0, 0}, frame.extent};
            vk.cmd_set_viewport(frame.command_buffer, 0, 1, &viewport);
            vk.cmd_set_scissor(frame.command_buffer, 0, 1, &scissor);
        }
        const VkDeviceSize offset = 0;
        if (fresh || vertices_ != vertices)
            vk.cmd_bind_vertex_buffers(frame.command_buffer, 0, 1, &vertices, &offset);
        if (fresh || indices_ != indices)
            vk.cmd_bind_index_buffer(frame.command_buffer, indices, 0, VK_INDEX_TYPE_UINT32);
        if (fresh || material_ != material)
            vk.cmd_bind_descriptor_sets(frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                layout, 0, 1, &material, 0, nullptr);
        if (enabled_)
        {
            command_ = frame.command_buffer; layout_ = layout; extent_ = frame.extent;
            pipeline_ = pipeline; vertices_ = vertices; indices_ = indices; material_ = material;
        }
    }

    void push(const ScenePassDispatch& vk, VkCommandBuffer command, VkPipelineLayout layout,
        VkShaderStageFlags stages, uint32_t offset, uint32_t size, const void* data)
    {
        if (!enabled_ || command_ != command || layout_ != layout || offset > 256 ||
            size > 256 - offset || (stages & ~(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)))
        {
            vk.cmd_push_constants(command, layout, stages, offset, size, data);
            // A different layout may invalidate compatibility of previously pushed values.
            valid_ = {};
            return;
        }
        bool same = true;
        for (unsigned stage = 0; stage < 2; ++stage)
            if (stages & (stage == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT))
                same &= std::all_of(valid_[stage].begin() + offset, valid_[stage].begin() + offset + size,
                    [](bool valid) { return valid; }) &&
                    std::memcmp(values_[stage].data() + offset, data, size) == 0;
        if (same) return;
        vk.cmd_push_constants(command, layout, stages, offset, size, data);
        for (unsigned stage = 0; stage < 2; ++stage)
            if (stages & (stage == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT))
            {
                std::copy_n(static_cast<const uint8_t*>(data), size, values_[stage].begin() + offset);
                std::fill_n(valid_[stage].begin() + offset, size, true);
            }
    }
private:
    bool enabled_{};
    VkCommandBuffer command_{};
    VkPipelineLayout layout_{};
    VkExtent2D extent_{};
    VkPipeline pipeline_{};
    VkBuffer vertices_{}, indices_{};
    VkDescriptorSet material_{};
    std::array<std::array<uint8_t, 256>, 2> values_{};
    std::array<std::array<bool, 256>, 2> valid_{};
};
}
