#pragma once

#include "LevelModels.h"

#include <cstdint>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace xray::render::vulkan
{
// One pair of Vulkan buffers holds many complete meshes. Each index remains
// local to its mesh; vkCmdDrawIndexed adds first_vertex as vertexOffset.
// Keeping index values local preserves OGF sliding-window offsets and bounds.
struct PackedLevelGeometry
{
    static constexpr size_t TargetBytes = 16u * 1024u * 1024u;
    std::vector<LevelVertex> vertices;
    std::vector<uint32_t> indices;

    bool would_flush(const LevelModel& model) const
    {
        return !vertices.empty() &&
            (vertices.size() * sizeof(LevelVertex) + indices.size() * sizeof(uint32_t) +
             model.vertices.size() * sizeof(LevelVertex) +
             model.indices.size() * sizeof(uint32_t) > TargetBytes);
    }

    bool append(const LevelModel& model, uint32_t& first_vertex,
        uint32_t& first_index, std::string& error)
    {
        if (model.vertices.empty() || model.indices.empty() ||
            model.vertices.size() > size_t(INT32_MAX) ||
            vertices.size() > size_t(INT32_MAX) - model.vertices.size() ||
            model.indices.size() > size_t(UINT32_MAX) ||
            indices.size() > size_t(UINT32_MAX) - model.indices.size())
        {
            error = "level geometry batch has empty mesh or index/vertex offset overflow";
            return false;
        }
        first_vertex = static_cast<uint32_t>(vertices.size());
        first_index = static_cast<uint32_t>(indices.size());
        vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
        indices.insert(indices.end(), model.indices.begin(), model.indices.end());
        return true;
    }
    void reset()
    {
        std::vector<LevelVertex>().swap(vertices);
        std::vector<uint32_t>().swap(indices);
    }
};
}
