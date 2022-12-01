#pragma once

#include "xrAICore/Navigation/level_graph_space.h"

namespace LevelGraph
{
class CLevelGraphManager
{
    bool m_compatibility_mode{};
    CLevelVertex* m_nodes; // nodes array
    size_t m_vertex_count;

public:
    CLevelGraphManager(IReader* stream, size_t vertex_count, u32 version) : m_vertex_count(vertex_count)
    {
        if (version == XRAI_VERSION_COP || version == XRAI_VERSION_CS)
        {
            m_nodes = static_cast<CLevelVertex*>(stream->pointer());
        }
        else if (version == XRAI_VERSION_SOC)
        {
            m_compatibility_mode = true;
            m_nodes = new CLevelVertex[vertex_count + 1]; // additional one, so we don't trigger access violation
            NodeCompressed8* oldNodes = static_cast<NodeCompressed8*>(stream->pointer());

            for (size_t i = 0; i < vertex_count; ++i)
            {
                NodeCompressed8& oldNode = oldNodes[i];
                NodeCompressed& newNode = m_nodes[i];
                newNode = oldNode;
            }

            // Mark end node as END NODE SOC
            // so we can spot that in debugger, if we need
            NodeCompressed& endNode = m_nodes[vertex_count + 1];
            endNode.data[0]  = 'E';
            endNode.data[1]  = 'N';
            endNode.data[2]  = 'D';
            endNode.data[3]  = ' ';
            endNode.data[4]  = 'N';
            endNode.data[5]  = 'O';
            endNode.data[6]  = 'D';
            endNode.data[7]  = 'E';
            endNode.data[8]  = ' ';
            endNode.data[9]  = 'S';
            endNode.data[10] = 'O';
            endNode.data[11] = 'C';
            static_assert(sizeof(endNode.data) == 12, "If you have changed the NodeCompressed structure, please update the code above.");
        }
        else
        {
            FATAL("Unsupported level graph version.");
        }
    }

    ~CLevelGraphManager()
    {
        if (m_compatibility_mode)
        {
            delete[] m_nodes;
        }
    }
    
    [[nodiscard]] CLevelVertex* begin() { return m_nodes; }
    [[nodiscard]] CLevelVertex* end() { return m_nodes + m_vertex_count; }
};
} // namespace LevelGraph
