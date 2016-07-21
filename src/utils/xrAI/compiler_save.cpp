#include "stdafx.h"
#include "compiler.h"
#include "guid_generator.h"

IC BYTE compress(float c, int max_value)
{
    int cover = iFloor(c * float(max_value) + .5f);
    clamp(cover, 0, max_value);
    return BYTE(cover);
}

struct CNodeCompressed
{
    IC void compress_node(NodeCompressed& Dest, vertex& Src);
};

IC void CNodeCompressed::compress_node(NodeCompressed& Dest, vertex& Src)
{
    Dest.light(15);
    for (u8 L = 0; L < 4; ++L)
        Dest.link(L, Src.n[L]);
}

void Compress(NodeCompressed& Dest, vertex& Src, hdrNODES& H)
{
    // Compress plane (normal)
    Dest.plane = pvCompress(Src.Plane.n);
    // Compress position
    CNodePositionCompressor(Dest.p, Src.Pos, H);
    // Light & Cover
    CNodeCompressed().compress_node(Dest, Src);
    Dest.high.cover0 = compress(Src.high_cover[0], 15);
    Dest.high.cover1 = compress(Src.high_cover[1], 15);
    Dest.high.cover2 = compress(Src.high_cover[2], 15);
    Dest.high.cover3 = compress(Src.high_cover[3], 15);
    Dest.low.cover0 = compress(Src.low_cover[0], 15);
    Dest.low.cover1 = compress(Src.low_cover[1], 15);
    Dest.low.cover2 = compress(Src.low_cover[2], 15);
    Dest.low.cover3 = compress(Src.low_cover[3], 15);
}

float CalculateHeight(Fbox& BB)
{
    // All nodes
    BB.invalidate();

    for (u32 i = 0; i < g_nodes.size(); i++)
    {
        vertex& N = g_nodes[i];
        BB.modify(N.Pos);
    }
    return BB.vMax.y - BB.vMin.y + EPS_L;
}

xr_vector<NodeCompressed> compressed_nodes;

class CNodeRenumberer
{
    IC bool operator=(const CNodeRenumberer&) {}
    struct SSortNodesPredicate
    {
        IC bool operator()(const NodeCompressed& vertex0, const NodeCompressed& vertex1) const
        {
            return (vertex0.p.xz() < vertex1.p.xz());
        }

        IC bool operator()(u32 vertex_id0, u32 vertex_id1) const
        {
            return (compressed_nodes[vertex_id0].p.xz() < compressed_nodes[vertex_id1].p.xz());
        }
    };

    xr_vector<NodeCompressed>& m_nodes;
    xr_vector<u32>& m_sorted;
    xr_vector<u32>& m_renumbering;

public:
    CNodeRenumberer(xr_vector<NodeCompressed>& nodes, xr_vector<u32>& sorted, xr_vector<u32>& renumbering)
        : m_nodes(nodes), m_sorted(sorted), m_renumbering(renumbering)
    {
        u32 N = (u32)m_nodes.size();
        m_sorted.resize(N);
        m_renumbering.resize(N);

        for (u32 i = 0; i < N; ++i)
            m_sorted[i] = i;

        std::stable_sort(m_sorted.begin(), m_sorted.end(), SSortNodesPredicate());

        for (u32 i = 0; i < N; ++i)
            m_renumbering[m_sorted[i]] = i;

        for (u32 i = 0; i < N; ++i)
        {
            for (u32 j = 0; j < 4; ++j)
            {
                u32 vertex_id = m_nodes[i].link(u8(j));
                if (vertex_id >= N)
                    continue;
                m_nodes[i].link(u8(j), m_renumbering[vertex_id]);
            }
        }

        std::stable_sort(m_nodes.begin(), m_nodes.end(), SSortNodesPredicate());
    }
};

void xrSaveNodes(LPCSTR name, LPCSTR out_name)
{
    Msg("NS: %d, CNS: %d, ratio: %f%%", sizeof(vertex), sizeof(CLevelGraph::CVertex),
        100 * float(sizeof(CLevelGraph::CVertex)) / float(sizeof(vertex)));

    Msg("Renumbering nodes...");

    string_path fName;
    strconcat(sizeof(fName), fName, name, out_name);

    IWriter* fs = FS.w_open(fName);

    // Header
    Logger.Status("Saving header...");
    hdrNODES H;
    H.version = XRAI_CURRENT_VERSION;
    H.count = g_nodes.size();
    H.size = g_params.fPatchSize;
    H.size_y = CalculateHeight(H.aabb);
    H.guid = generate_guid();
    fs->w(&H, sizeof(H));
    // All nodes
    Logger.Status("Saving nodes...");
    for (u32 i = 0; i < g_nodes.size(); ++i)
    {
        vertex& N = g_nodes[i];
        NodeCompressed NC;
        Compress(NC, N, H);
        compressed_nodes.push_back(NC);
    }

    xr_vector<u32> sorted;
    xr_vector<u32> renumbering;
    CNodeRenumberer A(compressed_nodes, sorted, renumbering);

    for (u32 i = 0; i < g_nodes.size(); ++i)
    {
        fs->w(&compressed_nodes[i], sizeof(NodeCompressed));
        Logger.Progress(float(i) / float(g_nodes.size()));
    }
    // Stats
    u32 SizeTotal = fs->tell();
    Msg("%dK saved", SizeTotal / 1024);

    FS.w_close(fs);
}
