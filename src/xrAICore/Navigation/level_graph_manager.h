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
        switch ((xrAI_Versions)version)
        {
        case XRAI_CURRENT_VERSION:
            static_assert(XRAI_CURRENT_VERSION == XRAI_VERSION_CS_COP,
                "If you have changed the xrAI version, don't forget to add back compatibility with CS/COP xrAI version.");
            m_nodes = static_cast<CLevelVertex*>(stream->pointer());
            break;

        case XRAI_VERSION_PRIQUEL:
        case XRAI_VERSION_SOC:
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

            // Mark end node
            // so we can spot that in debugger, if we need
            NodeCompressed& endNode = m_nodes[vertex_count + 1];
            endNode.data[0]  = 'A';
            endNode.data[1]  = 'I';
            endNode.data[2]  = version;
            endNode.data[3]  = ' ';
            endNode.data[4]  = 'N';
            endNode.data[5]  = 'O';
            endNode.data[6]  = 'D';
            endNode.data[7]  = 'E';
            endNode.data[8]  = ' ';
            endNode.data[9]  = 'E';
            endNode.data[10] = 'N';
            endNode.data[11] = 'D';
            static_assert(sizeof(endNode.data) == 12, "If you have changed the NodeCompressed structure, please update the code above.");
            break;
        }

        default:
            FATAL("Unsupported level graph version.");
            NODEFAULT;
        } // switch (version)
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
