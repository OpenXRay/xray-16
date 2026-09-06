#define SM_5_0
#include "common.h"
#include "local_shadow_common.h"

#define LOCAL_RESERVE_THREADS 256

cbuffer LocalShadowBinParams : register(b5)
{
    uint g_CandCount;
    uint g_NodeCount;
    uint g_IncludeAT;
    float g_ErrK;
    uint g_CapOpaque;
    uint g_CapTerrain;
    uint g_CapAT;
    uint g_Budget;
    uint g_Frame;
    uint g_BinPad0;
    uint g_BinPad1;
    uint g_BinPad2;
};

StructuredBuffer<LocalShadowView> g_Request : register(t15);
StructuredBuffer<uint4> g_CandList : register(t16);
StructuredBuffer<uint4> g_TileCount : register(t22);

RWStructuredBuffer<LocalShadowView> g_TileState : register(u0);
RWStructuredBuffer<uint4> g_Schedule : register(u1);
RWStructuredBuffer<uint> g_DirtyList : register(u2);
RWStructuredBuffer<uint> g_RefreshDyn : register(u3);
RWStructuredBuffer<uint4> g_PairBase : register(u4);
RWByteAddressBuffer g_EmitArgs : register(u5);
RWByteAddressBuffer g_ClearArgs : register(u6);
RWStructuredBuffer<uint> g_Stats : register(u7);

groupshared uint gs_want[LOCAL_RESERVE_THREADS];
groupshared uint gs_utd[LOCAL_RESERVE_THREADS];
groupshared uint gs_age[LOCAL_RESERVE_THREADS];
groupshared uint gs_isNew[LOCAL_RESERVE_THREADS];
groupshared uint gs_rank[LOCAL_RESERVE_THREADS];
groupshared uint gs_group[LOCAL_RESERVE_THREADS];
groupshared uint gs_slot[LOCAL_RESERVE_THREADS];
groupshared uint3 gs_cnt[LOCAL_RESERVE_THREADS];
groupshared uint gs_acc[LOCAL_RESERVE_THREADS];
groupshared uint3 gs_base[LOCAL_RESERVE_THREADS];
groupshared uint gs_index[LOCAL_RESERVE_THREADS];
groupshared uint gs_dyn[LOCAL_RESERVE_THREADS];
groupshared uint gs_at[LOCAL_RESERVE_THREADS];
groupshared uint gs_wanted;
groupshared uint gs_maxAge;
groupshared uint gs_accepted;
groupshared uint gs_dynCount;

[numthreads(LOCAL_RESERVE_THREADS, 1, 1)]
void main(uint3 gtID : SV_GroupThreadID)
{
    uint t = gtID.x;
    bool have = t < g_CandCount;

    gs_want[t] = 0u;
    gs_utd[t] = 0u;
    gs_age[t] = 0u;
    gs_isNew[t] = 0u;
    gs_rank[t] = 0u;
    gs_group[t] = 0u;
    gs_slot[t] = 0u;
    gs_cnt[t] = uint3(0u, 0u, 0u);
    gs_acc[t] = 0u;
    gs_base[t] = uint3(0u, 0u, 0u);
    gs_index[t] = 0u;
    gs_dyn[t] = 0u;
    gs_at[t] = 0u;
    if (t == 0u)
    {
        gs_wanted = 0u;
        gs_maxAge = 0u;
        gs_accepted = 0u;
        gs_dynCount = 0u;
    }
    GroupMemoryBarrierWithGroupSync();

    uint4 cand = g_CandList[have ? t : 0u];
    uint slot = have ? cand.x : 0u;
    LocalShadowView st = g_TileState[slot];
    LocalShadowView req = g_Request[slot];
    uint4 sched = g_Schedule[slot];

    bool inView = ((cand.w >> 31u) & 1u) != 0u;
    bool dynDue = ((cand.w >> 30u) & 1u) != 0u;
    uint rank = cand.w & 0xFFFFu;

    bool valid = st.zparams.w > 0.5;
    bool sameOwner = valid && st.meta.y == cand.z;
    bool upToDate = sameOwner && st.meta.x == cand.y;
    bool isNew = !sameOwner;

    if (valid && st.meta.y != cand.z)
        st.zparams.w = 0.0;
    st.rect.w = req.rect.w;
    st.shape = req.shape;
    if (have)
        g_TileState[slot] = st;

    if (sched.x != cand.z)
        sched = uint4(cand.z, 0u, 0u, 0u);

    uint3 cnt = have ? g_TileCount[t].xyz : uint3(0u, 0u, 0u);
    bool want = have && inView && !upToDate;
    if (want && sched.z == 0u)
    {
        sched.y = g_Frame;
        sched.z = 1u;
    }
    if (!want)
        sched.z = 0u;
    uint age = want ? (g_Frame - sched.y) : 0u;

    gs_want[t] = want ? 1u : 0u;
    gs_utd[t] = (have && upToDate) ? 1u : 0u;
    gs_age[t] = age;
    gs_isNew[t] = isNew ? 1u : 0u;
    gs_rank[t] = rank;
    gs_group[t] = cand.z;
    gs_slot[t] = slot;
    gs_cnt[t] = cnt;
    GroupMemoryBarrierWithGroupSync();

    if (want)
    {
        uint order = 0u;
        for (uint oj = 0u; oj < uint(LOCAL_RESERVE_THREADS); ++oj)
        {
            if (oj == t || gs_want[oj] == 0u)
                continue;
            bool jNew = gs_isNew[oj] != 0u;
            bool before = (gs_age[oj] > age)
                || (gs_age[oj] == age && jNew && !isNew)
                || (gs_age[oj] == age && jNew == isNew && gs_rank[oj] < rank);
            if (before)
                ++order;
        }
        gs_at[order] = t;
    }
    if (t == 0u)
    {
        uint w = 0u;
        uint mx = 0u;
        uint utd = 0u;
        for (uint wk = 0u; wk < uint(LOCAL_RESERVE_THREADS); ++wk)
        {
            utd += gs_utd[wk];
            if (gs_want[wk] == 0u)
                continue;
            ++w;
            mx = max(mx, gs_age[wk]);
        }
        gs_wanted = w;
        gs_maxAge = mx;
        g_Stats[3] = utd;
    }
    GroupMemoryBarrierWithGroupSync();

    if (t == 0u)
    {
        uint wanted = gs_wanted;
        uint3 sum = uint3(0u, 0u, 0u);
        uint3 demand = uint3(0u, 0u, 0u);
        uint accepted = 0u;
        uint deferred = 0u;
        uint skipped = 0u;
        uint ci = 0u;
        [loop] while (ci < wanted)
        {
            uint grp = gs_group[gs_at[ci]];
            uint3 gsum = uint3(0u, 0u, 0u);
            uint cj = ci;
            [loop] while (cj < wanted && gs_group[gs_at[cj]] == grp)
            {
                gsum += gs_cnt[gs_at[cj]];
                ++cj;
            }
            if (sum.x + gsum.x > g_CapOpaque || sum.y + gsum.y > g_CapTerrain || sum.z + gsum.z > g_CapAT)
            {
                demand = max(demand, gsum);
                skipped += cj - ci;
                ci = cj;
                continue;
            }
            if (accepted > 0u && (sum.x + sum.y + sum.z + gsum.x + gsum.y + gsum.z) > g_Budget)
            {
                deferred += wanted - ci;
                break;
            }
            for (uint ck = ci; ck < cj; ++ck)
            {
                uint lane = gs_at[ck];
                gs_acc[lane] = 1u;
                gs_base[lane] = sum;
                gs_index[lane] = accepted;
                ++accepted;
                sum += gs_cnt[lane];
            }
            ci = cj;
        }
        gs_accepted = accepted;
        g_Stats[0] = accepted;
        g_Stats[1] = deferred;
        g_Stats[2] = skipped;
        g_Stats[4] = sum.x;
        g_Stats[5] = sum.y;
        g_Stats[6] = sum.z;
        g_Stats[9] = demand.x;
        g_Stats[10] = demand.y;
        g_Stats[11] = demand.z;
        g_Stats[12] = gs_maxAge;
    }
    GroupMemoryBarrierWithGroupSync();

    bool acceptedTile = have && gs_acc[t] != 0u;
    if (acceptedTile)
    {
        LocalShadowView v = req;
        v.zparams.w = 1.0;
        v.meta.x = cand.y;
        v.meta.y = cand.z;
        g_TileState[slot] = v;
        g_DirtyList[gs_index[t]] = slot;
        g_PairBase[gs_index[t]] = uint4(gs_base[t], 0u);
        sched.z = 0u;
        sched.w = cand.y;
    }
    if (have)
        g_Schedule[slot] = sched;

    bool postValid = acceptedTile || sameOwner;
    gs_dyn[t] = (have && postValid && (acceptedTile || dynDue)) ? 1u : 0u;
    GroupMemoryBarrierWithGroupSync();

    if (t == 0u)
    {
        uint n = 0u;
        for (uint ei = 0u; ei < uint(LOCAL_RESERVE_THREADS); ++ei)
        {
            if (gs_dyn[ei] == 0u)
                continue;
            g_RefreshDyn[n] = gs_slot[ei];
            ++n;
        }
        gs_dynCount = n;
        g_EmitArgs.Store4(0, uint4(gs_accepted, 1u, 1u, 0u));
        g_ClearArgs.Store4(0, uint4(6u, gs_accepted, 0u, 0u));
        g_ClearArgs.Store4(16, uint4(6u, n, 0u, 0u));
        g_Stats[13] = n;
    }
}
