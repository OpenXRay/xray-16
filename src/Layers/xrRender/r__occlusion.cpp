#include "stdafx.h"
#include "r__occlusion.h"

#include "QueryHelper.h"

namespace xray::render::RENDER_NAMESPACE
{
R_occlusion::~R_occlusion(void) { occq_destroy(); }
void R_occlusion::occq_create(u32 limit)
{
    ZoneScoped;
    enabled = strstr(Core.Params, "-no_occq") ? false : true;
    pool.reserve(limit);
    used.reserve(limit);
    fids.reserve(limit);
    for (u32 it = 0; it < limit; it++)
    {
        Query q;
        q.order = it;
        if (FAILED(CreateQuery(&q.Q, D3D_QUERY_OCCLUSION)))
            break;
        pool.emplace_back(std::move(q));
    }
    std::reverse(pool.begin(), pool.end());
}
void R_occlusion::occq_destroy()
{
    for (const auto& q : used)
        ReleaseQuery(q.Q);
    for (const auto& q : pool)
        ReleaseQuery(q.Q);
    used.clear();
    pool.clear();
    fids.clear();
}

u32 R_occlusion::occq_begin(u32& ID)
{
    if (!enabled)
        return 0;

    ScopeLock lock{ &render_lock };

    if (pool.empty())
    {
        const auto sz = used.size();
        Query q;
        q.order = static_cast<u32>(sz);
        if (FAILED(CreateQuery(&q.Q, D3D_QUERY_OCCLUSION)))
        {
            if ((Device.dwFrame % 40) == 0)
                Msg(" RENDER [Warning]: Too many occlusion queries were issued (>%zu)!!!", sz);
            ID = iInvalidHandle;
            return 0;
        }
        if (sz == used.capacity())
        {
            used.reserve(sz + occq_size_base);
            pool.reserve(sz + occq_size_base);
        }
        pool.emplace(pool.begin(), std::move(q));
    }

    if (!fids.empty())
    {
        ID = fids.back();
        fids.pop_back();
        used[ID] = std::move(pool.back());
    }
    else
    {
        ID = static_cast<u32>(used.size());
        used.emplace_back(std::move(pool.back()));
    }
    pool.pop_back();
    if (FAILED(BeginQuery(used[ID].Q)))
    {
#if defined(USE_OGL)
        if (used[ID].Q && !glIsQuery(used[ID].Q))
        {
            ReleaseQuery(used[ID].Q);
            used[ID].Q = 0;
        }
#endif
        if (used[ID].Q)
            pool.emplace_back(std::move(used[ID]));
        used[ID].Q = 0;
        fids.emplace_back(ID);
        ID = iInvalidHandle;
        return 0;
    }
    RImplementation.BasicStats.OcclusionQueries++;

    return used[ID].order;
}
void R_occlusion::occq_end(u32& ID)
{
    if (!enabled || ID == iInvalidHandle)
        return;

    ScopeLock lock{ &render_lock };

    CHK_DX(EndQuery(used[ID].Q));
}
R_occlusion::occq_result R_occlusion::occq_get(u32& ID)
{
    if (!enabled || ID == iInvalidHandle)
        return 0xffffffff;

    ScopeLock lock{ &render_lock };

    occq_result fragments = 0;
    HRESULT hr;
    CTimer T;
    T.Start();
    RImplementation.BasicStats.Wait.Begin();
    while ((hr = GetData(used[ID].Q, &fragments, sizeof(fragments))) == S_FALSE)
    {
#if defined(USE_OGL)
        if (GLAD_GL_ES_VERSION_3_0)
        {
            fragments = static_cast<occq_result>(-1);
            break;
        }
#endif
        if (!SwitchToThread())
            Sleep(ps_r2_wait_sleep);

        if (T.GetElapsed_ms() > 500)
        {
            fragments = (occq_result)-1; // 0xffffffff;
            break;
        }
    }
    if (FAILED(hr))
        fragments = static_cast<occq_result>(-1);
    RImplementation.BasicStats.Wait.End();

    if (0 == fragments)
        RImplementation.BasicStats.OcclusionCulled++;

    // insert into pool (sorting in decreasing order)
    Query& Q = used[ID];
    if (FAILED(hr))
    {
        ReleaseQuery(Q.Q);
#if defined(XR_PLATFORM_ANDROID)
        static u32 lastInvalidQueryReport = 0;
        if (Device.dwTimeContinual - lastInvalidQueryReport >= 5000)
        {
            Msg("! [render-query] retired invalid occlusion query; result treated as visible");
            lastInvalidQueryReport = Device.dwTimeContinual;
        }
#endif
    }
    else if (pool.empty())
        pool.emplace_back(Q);
    else
    {
        int it = int(pool.size()) - 1;
        while ((it >= 0) && (pool[it].order < Q.order))
            it--;
        pool.emplace(pool.begin() + it + 1, std::move(Q));
    }

    // remove from used and shrink as nesessary
    used[ID].Q = 0;
    fids.emplace_back(ID);
    ID = 0;
    return fragments;
}
} // namespace xray::render::RENDER_NAMESPACE
