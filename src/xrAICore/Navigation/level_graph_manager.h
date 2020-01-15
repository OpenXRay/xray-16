#pragma once

#include "xrAICore/Navigation/level_graph_space.h"

namespace LevelGraph
{
class CLevelGraphManager
{
    bool compatibilityMode;
    xr_vector<CLevelVertex*> m_nodes; // nodes array

public:
    CLevelGraphManager(IReader* stream, size_t vertex_count, u32 version)
    {
        m_nodes.resize(vertex_count);
        if (version <= 8)
        {
            compatibilityMode = true;
            NodeCompressedOld* nodes = static_cast<NodeCompressedOld*>(stream->pointer());
            CLevelVertex* newNodes = new CLevelVertex[vertex_count];
            for (size_t i = 0; i < vertex_count; ++i)
            {
                CLevelVertex& vertex = newNodes[i];
                NodeCompressed& newNode = vertex;
                NodeCompressedOld& oldNode = nodes[i];

                CopyMemory(newNode.data, oldNode.data, sizeof(oldNode.data) / sizeof(u8));
                newNode.high = oldNode.cover;
                newNode.low = oldNode.cover;
                newNode.plane = oldNode.plane;
                newNode.p = oldNode.p;

                m_nodes[i] = &vertex;
            }
        }
        else
        {
            compatibilityMode = false;
            CLevelVertex* begin = static_cast<CLevelVertex*>(stream->pointer());
            CLevelVertex* end = begin + vertex_count;
            for (size_t i = 0; begin != end; ++begin, ++i)
            {
                m_nodes[i] = begin;
            }
        }
    }

    ~CLevelGraphManager()
    {
        if (compatibilityMode)
        {
            for (auto& node : m_nodes)
                xr_delete(node);
        }
        m_nodes.clear();
    }

    [[nodiscard]] CLevelVertex* front() { return m_nodes.front(); }
    [[nodiscard]] CLevelVertex* back() { return m_nodes.back(); }

    [[nodiscard]] auto begin() { return m_nodes.begin(); }
    [[nodiscard]] auto end() { return m_nodes.end(); }

    [[nodiscard]] auto cbegin() const { return m_nodes.cbegin(); }
    [[nodiscard]] auto cend() const { return m_nodes.cend(); }

    [[nodiscard]] CLevelVertex* at(size_t id) { VERIFY(id < size()); return m_nodes[id]; }
    [[nodiscard]] CLevelVertex* operator[](size_t id) { return m_nodes[id]; }
    
    [[nodiscard]] CLevelVertex* operator+(size_t id) { return m_nodes[id]; }

    [[nodiscard]] bool empty() const { return m_nodes.empty(); }
    [[nodiscard]] size_t size() const { return m_nodes.size(); }
};
} // namespace LevelGraph
