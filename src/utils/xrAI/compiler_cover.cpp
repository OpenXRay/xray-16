#include "stdafx.h"
#include "compiler.h"
#include "xrCDB/Intersect.hpp"
#include <mmsystem.h>

#include "xrGame/quadtree.h"
#include "xrGame/cover_point.h"
#include "Common/object_broker.h"
#include "xrCommon/xr_unordered_map.h"

Shader_xrLC_LIB* g_shaders_xrlc;
xr_vector<b_material> g_materials;
//xr_vector<b_shader> g_shader_render;
//xr_vector<b_shader> g_shader_compile;
xr_vector<b_BuildTexture> g_textures;
xr_vector<b_rc_face> g_rc_faces;

typedef xr_vector<bool> COVER_NODES;
COVER_NODES g_cover_nodes;

// -------------------------------- Ray pick
typedef Fvector RayCache[3];

IC float getLastRP_Scale(CDB::COLLIDER* DB, RayCache& C)
{
    float scale = 1.f;

    for (u32 I = 0; I < DB->r_count(); I++)
    {
        CDB::RESULT& rpinf = DB->r_begin()[I];
        // Access to texture
        b_rc_face& F = g_rc_faces[rpinf.id];

        if (F.dwMaterial >= g_materials.size())
            Logger.clMsg("[%d] -> [%d]", F.dwMaterial, g_materials.size());
        b_material& M = g_materials[F.dwMaterial];
        b_texture& T = g_textures[M.surfidx];
        Shader_xrLCVec& LIB = g_shaders_xrlc->Library();
        if (M.shader_xrlc >= LIB.size())
            return 0; //. hack
        Shader_xrLC& SH = LIB[M.shader_xrlc];
        if (!SH.flags.bLIGHT_CastShadow)
            continue;

        if (T.pSurface == nullptr || T.bHasAlpha == false)
        {
            T.bHasAlpha = false;

            // Opaque poly - cache it
            C[0].set(rpinf.verts[0]);
            C[1].set(rpinf.verts[1]);
            C[2].set(rpinf.verts[2]);
            return 0;
        }

        // barycentric coords
        // note: W,U,V order
        Fvector B;
        B.set(1.0f - rpinf.u - rpinf.v, rpinf.u, rpinf.v);

        // calc UV
        auto cuv = F.t;
        Fvector2 uv;
        uv.x = cuv[0].x * B.x + cuv[1].x * B.y + cuv[2].x * B.z;
        uv.y = cuv[0].y * B.x + cuv[1].y * B.y + cuv[2].y * B.z;

        int U = iFloor(uv.x * float(T.dwWidth) + .5f);
        int V = iFloor(uv.y * float(T.dwHeight) + .5f);
        U %= T.dwWidth;
        if (U < 0)
            U += T.dwWidth;
        V %= T.dwHeight;
        if (V < 0)
            V += T.dwHeight;

        u32 pixel = T.pSurface[V * T.dwWidth + U];
        u32 pixel_a = color_get_A(pixel);
        float opac = 1.f - float(pixel_a) / 255.f;
        scale *= opac;
    }

    return scale;
}

IC float rayTrace(CDB::COLLIDER* DB, Fvector& P, Fvector& D, float R, RayCache& C)
{
    R_ASSERT(DB);

    // 1. Check cached polygon
    float _u, _v, range;
    bool res = CDB::TestRayTri(P, D, C, _u, _v, range, false);
    if (res)
    {
        if (range > 0 && range < R)
            return 0;
    }

    // 2. Polygon doesn't pick - real database query
    DB->ray_query(CDB::OPT_CULL, &Level, P, D, R);

    // 3. Analyze polygons and cache nearest if possible
    if (0 == DB->r_count())
    {
        return 1;
    }
    else
    {
        return getLastRP_Scale(DB, C);
    }
}

IC int calcSphereSector(Fvector& dir)
{
    Fvector2 flat;

    // flatten
    flat.set(dir.x, dir.z);
    flat.norm();

    // analyze
    if (_abs(flat.x) > _abs(flat.y))
    {
        // sector 0,7,3,4
        if (flat.x < 0)
        {
            // sector 3,4
            if (flat.y > 0)
                return 3;
            else
                return 4;
        }
        else
        {
            // sector 0,7
            if (flat.y > 0)
                return 0;
            else
                return 7;
        }
    }
    else
    {
        // sector 1,2,6,5
        if (flat.x < 0)
        {
            // sector 2,5
            if (flat.y > 0)
                return 2;
            else
                return 5;
        }
        else
        {
            // sector 1,6
            if (flat.y > 0)
                return 1;
            else
                return 6;
        }
    }
}

// volumetric query
using Nearest = xr_vector<u32>;

class Query
{
public:
    Nearest q_List;
    Nearest q_Clear;
    xr_unordered_map<u32, bool> q_Marks;
    Fvector q_Base;

    IC void Begin(const int count)
    {
        q_List.reserve(8192);
        q_Clear.reserve(8192);
        q_Marks.reserve(count);
    }

    IC void Init(const Fvector& P)
    {
        q_Base.set(P);
        q_List.clear();
        q_Clear.clear();
    }

    IC void Perform(u32 ID)
    {
        if (ID == InvalidNode)
            return;
        if (q_Marks[ID])
            return;

        q_Marks[ID] = true;
        q_Clear.push_back(ID);

        vertex& N = g_nodes[ID];
        if (q_Base.distance_to_sqr(N.Pos) > cover_sqr_dist)
            return;

        // ok
        q_List.push_back(ID);

        Perform(N.n1);
        Perform(N.n2);
        Perform(N.n3);
        Perform(N.n4);
    }

    IC void Clear()
    {
        q_Marks.clear();
    }
};
struct RC
{
    RayCache C;
};

class CoverThread : public CThread
{
    u32 Nstart, Nend;
    xr_vector<RC> cache;
    CDB::COLLIDER DB;
    Query Q;

    typedef float Cover[4];

public:
    CoverThread(u32 ID, u32 _start, u32 _end) : CThread(ID, ProxyMsg), Q()
    {
        Nstart = _start;
        Nend = _end;
    }

    void compute_cover_value(u32 const& N, vertex& BaseNode, float const& cover_height, Cover& cover)
    {
        Fvector& BasePos = BaseNode.Pos;
        Fvector TestPos = BasePos;
        TestPos.y += cover_height;

        int   c_total[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        float c_passed[8] = {0, 0, 0, 0, 0, 0, 0, 0};

        // perform volumetric query
        Q.Init(BasePos);
        Q.Perform(N);

        // main cycle: trace rays and compute counts
        for (auto &it : Q.q_List)
        {
            // calc dir & range
            u32 ID = it;
            R_ASSERT(ID < g_nodes.size());
            if (N == ID)
                continue;
            vertex& N2 = g_nodes[ID];
            Fvector& Pos = N2.Pos;
            Fvector Dir;
            Dir.sub(Pos, BasePos);
            float range = Dir.magnitude();
            Dir.div(range);

            // raytrace
            int sector = calcSphereSector(Dir);
            c_total [sector] += 1;
            c_passed[sector] += rayTrace(&DB, TestPos, Dir, range, cache[ID].C); //
        }
        Q.Clear();

        // analyze probabilities
        float value[8];
        for (int dirs = 0; dirs < 8; dirs++)
        {
            R_ASSERT(c_passed[dirs] <= c_total[dirs]);
            if (c_total[dirs] == 0)
                value[dirs] = 0;
            else
                value[dirs] = c_passed[dirs] / float(c_total[dirs]);
            clamp(value[dirs], 0.f, 1.f);
        }

        cover[0] = (value[2] + value[3] + value[4] + value[5]) / 4.f;
        clamp(cover[0], 0.f, 1.f); // left
        cover[1] = (value[0] + value[1] + value[2] + value[3]) / 4.f;
        clamp(cover[1], 0.f, 1.f); // forward
        cover[2] = (value[6] + value[7] + value[0] + value[1]) / 4.f;
        clamp(cover[2], 0.f, 1.f); // right
        cover[3] = (value[4] + value[5] + value[6] + value[7]) / 4.f;
        clamp(cover[3], 0.f, 1.f); // back
    }

    virtual void Execute()
    {
        {
            RC rc;
            rc.C[0].set(0, 0, 0);
            rc.C[1].set(0, 0, 0);
            rc.C[2].set(0, 0, 0);

            cache.assign(g_nodes.size() * 2, rc);
        }

        FPU::m24r();

        Q.Begin(g_nodes.size());
        for (u32 N = Nstart; N < Nend; N++)
        {
            // initialize process
            thProgress = float(N - Nstart) / float(Nend - Nstart);
            vertex& BaseNode = g_nodes[N];

            if (!g_cover_nodes[N])
            {
                BaseNode.high_cover[0] = flt_max;
                BaseNode.high_cover[1] = flt_max;
                BaseNode.high_cover[2] = flt_max;
                BaseNode.high_cover[3] = flt_max;
                BaseNode.low_cover[0] = flt_max;
                BaseNode.low_cover[1] = flt_max;
                BaseNode.low_cover[2] = flt_max;
                BaseNode.low_cover[3] = flt_max;
                continue;
            }

            compute_cover_value(N, BaseNode, high_cover_height, BaseNode.high_cover);
            compute_cover_value(N, BaseNode, low_cover_height, BaseNode.low_cover);
        }
    }
};

bool valid_vertex_id(const u32& vertex_id) { return (vertex_id != InvalidNode); }
bool cover(const vertex& v, u32 index0, u32 index1)
{
    return (valid_vertex_id(v.n[index0]) && valid_vertex_id(g_nodes[v.n[index0]].n[index1]));
}

bool critical_point(const vertex& v, u32 index, u32 index0, u32 index1)
{
    return (!valid_vertex_id(v.n[index]) && (!valid_vertex_id(v.n[index0]) || !valid_vertex_id(v.n[index1]) ||
                                                   cover(v, index0, index) || cover(v, index1, index)));
}

bool is_cover(const vertex& v)
{
    return (critical_point(v, 0, 1, 3)
         || critical_point(v, 2, 1, 3)
         || critical_point(v, 1, 0, 2)
         || critical_point(v, 3, 0, 2));
}

void compute_cover_nodes()
{
    g_cover_nodes.assign(g_nodes.size(), false);

    auto J = g_cover_nodes.begin();
    for (auto &I : g_nodes)
        *J++ = is_cover(I);
}

bool vertex_in_direction(const u32& start_vertex_id, const u32& target_vertex_id)
{
    const Fvector& finish_position = g_nodes[target_vertex_id].Pos;
    u32 cur_vertex_id = start_vertex_id, prev_vertex_id = u32(-1);
    Fbox2 box;
    Fvector2 identity, start, dest, dir;

    identity.x = identity.y = g_params.fPatchSize * .5f;
    const Fvector& start_position = g_nodes[start_vertex_id].Pos;
    start = Fvector2().set(start_position.x, start_position.z);
    dest.set(finish_position.x, finish_position.z);
    dir.sub(dest, start);
    auto temp = start;

    float cur_sqr = _sqr(temp.x - dest.x) + _sqr(temp.y - dest.y);
    for (;;)
    {
        bool found = false;
        for (int I = 0, E = 4; I != E; ++I)
        {
            u32 next_vertex_id = g_nodes[cur_vertex_id].n[I];
            if ((next_vertex_id == prev_vertex_id) || !valid_vertex_id(next_vertex_id))
                continue;

            const Fvector& position = g_nodes[next_vertex_id].Pos;
            temp = Fvector2().set(position.x, position.z);
            box.min = box.max = temp;
            box.grow(identity);
            if (box.pick_exact(start, dir))
            {
                if (next_vertex_id == target_vertex_id)
                    return (true);

                Fvector2 temp2;
                temp2.add(box.min, box.max);
                temp2.mul(.5f);
                float dist = _sqr(temp2.x - dest.x) + _sqr(temp2.y - dest.y);
                if (dist > cur_sqr)
                    continue;

                cur_sqr = dist;
                found = true;
                prev_vertex_id = cur_vertex_id;
                cur_vertex_id = next_vertex_id;
                break;
            }
        }

        if (!found)
            return (false);
    }
}

extern float CalculateHeight(Fbox& BB);

void compute_non_covers()
{
    Fbox aabb;
    CalculateHeight(aabb);

    CQuadTree<CCoverPoint> covers(aabb, g_params.fPatchSize * .5f, 8 * 65536, 4 * 65536);

    auto J = g_cover_nodes.begin();
    for (auto &I : g_nodes)
    {
        if (!*J++)
            continue;

        if ((I.high_cover[0] + I.high_cover[1] + I.high_cover[2] + I.high_cover[3]) >= 4 * .999f)
            if ((I.low_cover[0] + I.low_cover[1] + I.low_cover[2] + I.low_cover[3]) >= 4 * .999f)
                continue;

        covers.insert(xr_new<CCoverPoint>(I.Pos, std::distance(&g_nodes.front(), &I)));
    }

    VERIFY(covers.size());

    using COVERS = xr_vector<CCoverPoint*>;
    using COVER_PAIR = std::pair<float, CCoverPoint*>;
    using COVER_PAIRS = xr_vector<COVER_PAIR>;

    COVERS nearest;
    COVER_PAIRS cover_pairs;

    auto K = g_cover_nodes.begin();
    for (auto &I : g_nodes)
    {
        if (*K++)
            continue;

        covers.nearest(I.Pos, cover_distance, nearest);
        if (nearest.empty())
        {
            for (int i = 0; i < 4; ++i)
            {
                VERIFY(I.high_cover[i] == flt_max);
                I.high_cover[i] = 1.f;

                VERIFY(I.low_cover[i] == flt_max);
                I.low_cover[i] = 1.f;
            }
            continue;
        }

        cover_pairs.clear();
        cover_pairs.reserve(nearest.size());

        float cumulative_weight = 0.f;

        for (auto &i : nearest)
        {
            if (!vertex_in_direction(std::distance(&g_nodes.front(), &I), i->level_vertex_id()))
                continue;

            float weight = 1.f / i->position().distance_to(I.Pos);
            cumulative_weight += weight;
            cover_pairs.emplace_back(weight, i);
        }

        // this is incorrect
        if (cover_pairs.empty())
        {
            for (int i = 0; i < 4; ++i)
            {
                VERIFY(I.high_cover[i] == flt_max);
                I.high_cover[i] = 1.f;

                VERIFY(I.low_cover[i] == flt_max);
                I.low_cover[i] = 1.f;
            }
            continue;
        }

        for (int j = 0; j < 4; ++j)
        {
            VERIFY(I.high_cover[j] == flt_max);
            I.high_cover[j] = 0.f;

            VERIFY(I.low_cover[j] == flt_max);
            I.low_cover[j] = 0.f;
        }

        for (auto &i : cover_pairs)
        {
            vertex& current = g_nodes[i.second->level_vertex_id()];
            float factor = i.first / cumulative_weight;
            for (int j = 0; j < 4; ++j)
            {
                I.high_cover[j] += factor * current.high_cover[j];
                I.low_cover[j] += factor * current.low_cover[j];
            }
        }

        for (int i2 = 0; i2 < 4; ++i2)
        {
            clamp(I.high_cover[i2], 0.f, 1.f);
            clamp(I.low_cover[i2], 0.f, 1.f);
        }
    }

    covers.all(nearest);
    delete_data(nearest);
}

extern void mem_Optimize();
void xrCover(bool pure_covers)
{
    Logger.Status("Calculating...");

    if (!pure_covers)
        compute_cover_nodes();
    else
        g_cover_nodes.assign(g_nodes.size(), true);

    // Start threads, wait, continue --- perform all the work
    CTimer timer;
    timer.Start();
    CThreadManager Threads(ProxyStatus, ProxyProgress);
    u32 stride = g_nodes.size() / NUM_THREADS;
    u32 last = g_nodes.size() - stride * (NUM_THREADS - 1);
    for (u32 thID = 0; thID < NUM_THREADS; thID++)
        Threads.start(
            xr_new<CoverThread>(thID, thID * stride, thID * stride + ((thID == (NUM_THREADS - 1)) ? last : stride)));
    Threads.wait();
    Logger.clMsg("%f seconds elapsed.", timer.GetElapsed_sec());

    if (!pure_covers)
    {
        compute_non_covers();
    }
    else
    {
        // Smooth
        Logger.Status("Smoothing coverage mask...");
        mem_Optimize();
        auto Old = g_nodes;
        for (u32 N = 0; N < g_nodes.size(); N++)
        {
            vertex& Base = Old[N];
            vertex& Dest = g_nodes[N];

            for (int dir = 0; dir < 4; dir++)
            {
                float val = 2 * Base.high_cover[dir];
                float val2 = 2 * Base.low_cover[dir];
                float cnt = 2;

                for (int nid = 0; nid < 4; nid++)
                {
                    if (Base.n[nid] != InvalidNode)
                    {
                        val += Old[Base.n[nid]].high_cover[dir];
                        val2 += Old[Base.n[nid]].low_cover[dir];
                        cnt += 1.f;
                    }
                }
                Dest.high_cover[dir] = val / cnt;
                Dest.low_cover[dir] = val2 / cnt;
            }
        }
    }
}
