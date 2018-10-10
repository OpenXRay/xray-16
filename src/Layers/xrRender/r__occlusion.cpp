#include "stdafx.h"
#include "r__occlusion.h"

#include "QueryHelper.h"

R_occlusion::R_occlusion(void) { enabled = strstr(Core.Params, "-no_occq") ? FALSE : TRUE; }
R_occlusion::~R_occlusion(void) { occq_destroy(); }
void R_occlusion::occq_create(u32 limit)
{
    pool.reserve(limit);
    used.reserve(limit);
    fids.reserve(limit);
    for (u32 it = 0; it < limit; it++)
    {
        _Q q;
        q.order = it;
        if (FAILED(CreateQuery(&q.Q)))
            break;
        pool.push_back(q);
    }
    std::reverse(pool.begin(), pool.end());
}
void R_occlusion::occq_destroy()
{
    while (!used.empty())
    {
        ReleaseQuery(used.back().Q);
        used.pop_back();
    }
    while (!pool.empty())
    {
        ReleaseQuery(pool.back().Q);
        pool.pop_back();
    }
    used.clear();
    pool.clear();
    fids.clear();
}

u32 R_occlusion::occq_begin(u32& ID)
{
    if (!enabled)
        return 0;

    //	Igor: prevent release crash if we issue too many queries
    if (pool.empty())
    {
        if ((Device.dwFrame % 40) == 0)
            Msg(" RENDER [Warning]: Too many occlusion queries were issued(>1536)!!!");
        ID = iInvalidHandle;
        return 0;
    }

    RImplementation.BasicStats.OcclusionQueries++;
    if (!fids.empty())
    {
        ID = fids.back();
        fids.pop_back();
        VERIFY(pool.size());
        used[ID] = pool.back();
    }
    else
    {
        ID = used.size();
        VERIFY(pool.size());
        used.push_back(pool.back());
    }
    pool.pop_back();
    // CHK_DX					(used[ID].Q->Issue	(D3DISSUE_BEGIN));
    CHK_DX(BeginQuery(used[ID].Q));

    // Msg				("begin: [%2d] - %d", used[ID].order, ID);

    return used[ID].order;
}
void R_occlusion::occq_end(u32& ID)
{
    if (!enabled)
        return;

    //	Igor: prevent release crash if we issue too many queries
    if (ID == iInvalidHandle)
        return;

    // Msg				("end  : [%2d] - %d", used[ID].order, ID);
    // CHK_DX			(used[ID].Q->Issue	(D3DISSUE_END));
    CHK_DX(EndQuery(used[ID].Q));
}
R_occlusion::occq_result R_occlusion::occq_get(u32& ID)
{
    if (!enabled)
        return 0xffffffff;

    //	Igor: prevent release crash if we issue too many queries
    if (ID == iInvalidHandle)
        return 0xFFFFFFFF;

    occq_result fragments = 0;
    HRESULT hr;
    // CHK_DX		(used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH));
    // Msg			("get  : [%2d] - %d => %d", used[ID].order, ID, fragments);
    CTimer T;
    T.Start();
    RImplementation.BasicStats.Wait.Begin();
    // while	((hr=used[ID].Q->GetData(&fragments,sizeof(fragments),D3DGETDATA_FLUSH))==S_FALSE) {
    VERIFY2(ID < used.size(), make_string("_Pos = %d, size() = %d ", ID, used.size()));
    while ((hr = GetData(used[ID].Q, &fragments, sizeof(fragments))) == S_FALSE)
    {
        if (!SwitchToThread())
            Sleep(ps_r2_wait_sleep);

        if (T.GetElapsed_ms() > 500)
        {
            fragments = (occq_result)-1; // 0xffffffff;
            break;
        }
    }
    RImplementation.BasicStats.Wait.End();
#ifndef USE_OGL
    if (hr == D3DERR_DEVICELOST)
        fragments = 0xffffffff;
#endif

    if (0 == fragments)
        RImplementation.BasicStats.OcclusionCulled++;

    // insert into pool (sorting in decreasing order)
    _Q& Q = used[ID];
    if (pool.empty())
        pool.push_back(Q);
    else
    {
        int it = int(pool.size()) - 1;
        while ((it >= 0) && (pool[it].order < Q.order))
            it--;
        pool.insert(pool.begin() + it + 1, Q);
    }

    // remove from used and shrink as nesessary
    used[ID].Q = 0;
    fids.push_back(ID);
    ID = 0;
    return fragments;
}
