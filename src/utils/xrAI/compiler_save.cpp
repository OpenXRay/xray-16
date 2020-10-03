#include "stdafx.h"
#include "compiler.h"
#include "guid_generator.h"

IC u8 compress(float c, int max_value)
{
    int cover = iFloor(c * float(max_value) + .5f);
    clamp(cover, 0, max_value);
    return u8(cover);
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

    for (auto &i : g_nodes)
        BB.modify(i.Pos);

    return BB.vMax.y - BB.vMin.y + EPS_L;
}

xr_vector<NodeCompressed> compressed_nodes;

class CNodeRenumberer
{
    struct SSortNodesPredicate
    {
        IC bool operator()(const NodeCompressed& vertex0, const NodeCompressed& vertex1) const
        {
            return (vertex0.p.xz() < vertex1.p.xz());
        }

        IC bool operator()(const u32 vertex_id0, const u32 vertex_id1) const
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
        u32 N = m_nodes.size();
        m_sorted.resize(N);
        m_renumbering.resize(N);

        for (u32 i = 0; i < N; ++i)
            m_sorted[i] = i;

        std::stable_sort(m_sorted.begin(), m_sorted.end(), SSortNodesPredicate());

        for (u32 i = 0; i < N; ++i)
            m_renumbering[m_sorted[i]] = i;

        for (auto &i : m_nodes)
        {
            for (u8 j = 0; j < 4; ++j)
            {
                u32 vertex_id = i.link(j);
                if (vertex_id >= N)
                    continue;
                i.link(j, m_renumbering[vertex_id]);
            }
        }

        std::stable_sort(m_nodes.begin(), m_nodes.end(), SSortNodesPredicate());
    }
};

void xrSaveNodes(LPCSTR name, LPCSTR out_name)
{
    Logger.Status("Saving nodes...");

    Logger.clMsg("NS: %d, CNS: %d, ratio: %f%%", sizeof(vertex), sizeof(CLevelGraph::CLevelVertex),
        100 * float(sizeof(CLevelGraph::CLevelVertex)) / float(sizeof(vertex)));

    string_path fName;
    strconcat(sizeof(fName), fName, name, out_name);
    IWriter* fs = FS.w_open(fName);

    // Header
    Logger.clMsg("Saving header...");
    hdrNODES H;
    H.version = XRAI_CURRENT_VERSION;
    H.count = g_nodes.size();
    H.size = g_params.fPatchSize;
    H.size_y = CalculateHeight(H.aabb);
    H.guid = generate_guid();
    fs->w(&H, sizeof(H));
    // All nodes
    Logger.clMsg("Compressing nodes...");
    compressed_nodes.reserve(g_nodes.size());
    for (auto &i : g_nodes)
    {
        NodeCompressed NC;
        Compress(NC, i, H);
        compressed_nodes.push_back(NC);
    }

    xr_vector<u32> sorted;
    xr_vector<u32> renumbering;
    Logger.clMsg("Renumbering nodes...");
    CNodeRenumberer A(compressed_nodes, sorted, renumbering);

    Logger.clMsg("Saving nodes...");
    for (auto &i : compressed_nodes)
        fs->w(&i, sizeof(NodeCompressed));

    // Stats
    u32 SizeTotal = fs->tell();
    Logger.clMsg("%dK saved", SizeTotal / 1024);

    FS.w_close(fs);
}
