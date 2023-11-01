#pragma once

#include "xrAICore/Navigation/level_graph_space.h"

namespace LevelGraph
{
class CLevelGraphManager
{
    bool m_compatibility_mode{};
    CLevelVertex* m_nodes; // nodes array
    size_t m_vertex_count;

private:
    template <typename OldNodes>
    CLevelVertex* convert_nodes(IReader* stream, size_t vertex_count, xrAI_Versions version)
    {
        m_compatibility_mode = true;

        auto* nodes = xr_alloc<NodeCompressed>(vertex_count + 1); // additional one, so we don't trigger access violation
        const auto* oldNodes = static_cast<OldNodes*>(stream->pointer());

        for (size_t i = 0; i < vertex_count; ++i)
        {
            nodes[i] = oldNodes[i];
        }

        // Mark end node
        // so we can spot that in debugger, if we need
        NodeCompressed& endNode = nodes[vertex_count];
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
        static_assert(sizeof(endNode.data) >= 12, "If you have changed the NodeCompressed structure, please update the code above.");

        static_assert(sizeof(CLevelVertex) == sizeof(NodeCompressed),
            "If you have changed the CLevelVertex class, please update the nodes allocation code above.");
        return static_cast<CLevelVertex*>(nodes);
    }

public:
    CLevelGraphManager(IReader* stream, size_t vertex_count, u32 version) : m_vertex_count(vertex_count)
    {
        switch ((xrAI_Versions)version)
        {
        case XRAI_CURRENT_VERSION:
            static_assert(XRAI_CURRENT_VERSION == XRAI_VERSION_SKYLOADER,
                "If you have changed the xrAI version, don't forget to add back compatibility older versions.");
            m_nodes = static_cast<CLevelVertex*>(stream->pointer());
            break;

        case XRAI_VERSION_BORSHT_BIG:
            m_nodes = convert_nodes<NodeCompressed12>(stream, vertex_count, (xrAI_Versions)version);
            break;

        case XRAI_VERSION_BORSHT:
            m_nodes = convert_nodes<NodeCompressed11>(stream, vertex_count, (xrAI_Versions)version);
            break;

        case XRAI_VERSION_CS_COP:
            m_nodes = convert_nodes<NodeCompressed10>(stream, vertex_count, (xrAI_Versions)version);
            break;

        case XRAI_VERSION_PRIQUEL:
        case XRAI_VERSION_SOC:
            m_nodes = convert_nodes<NodeCompressed7>(stream, vertex_count, (xrAI_Versions)version);
            break;

        default:
            FATAL("Unsupported level graph version.");
            NODEFAULT;
        } // switch (version)
    }

    ~CLevelGraphManager()
    {
        if (m_compatibility_mode)
        {
            xr_free(m_nodes);
        }
    }

    [[nodiscard]] CLevelVertex* begin() { return m_nodes; }
    [[nodiscard]] CLevelVertex* end() { return m_nodes + m_vertex_count; }
};
} // namespace LevelGraph
