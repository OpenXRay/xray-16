#include "stdafx.h"
#include "Skin4W_MT.hpp"
#include "Threading/ttapi.h"
#ifdef _EDITOR
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#else
#include "Animation/Bone.hpp"
#include "Layers/xrRender/SkeletonXVertRender.h"
#endif

namespace XRay
{
namespace Math
{

Skin4WFunc Skin4W_MTs = nullptr;

struct SkinParams
{
    void *Dest;
    void *Src;
    u32 Count;
    void *Data;
};

void Skin4W_Stream(void *params)
{
#ifdef _GPA_ENABLED	
    TAL_SCOPED_TASK_NAMED("Skin4W_Stream()");
#endif
    auto &sp = *(SkinParams*)params;
    auto dst = (vertRender*)sp.Dest;
    auto src = (vertBoned4W*)sp.Src;
    auto bones = (CBoneInstance*)sp.Data;
    Skin4W_MTs(dst, src, sp.Count, bones);
}

void Skin4W_MT(vertRender *dst, vertBoned4W *src, u32 vCount, CBoneInstance *bones)
{
#ifdef _GPA_ENABLED
    TAL_SCOPED_TASK_NAMED("Skin4W_MT()");
#endif
    u32 workerCount = ttapi_GetWorkerCount();
    if (vCount < workerCount * 64)
    {
        Skin4W_MTs(dst, src, vCount, bones);
        return;
    }
    auto params = (SkinParams*)_alloca(sizeof(SkinParams)*workerCount);
    // Give ~1% more for the last worker to minimize wait in final spin
    u32 nSlice = vCount / 128;
    u32 nStep = (vCount - nSlice) / workerCount;
    u32 nLast = vCount - nStep*(workerCount - 1);
    for (u32 i = 0; i < workerCount; i++)
    {
        params[i].Dest = dst + i*nStep;
        params[i].Src = src + i*nStep;
        params[i].Count = i == (workerCount - 1) ? nLast : nStep;
        params[i].Data = bones;
        ttapi_AddWorker(Skin4W_Stream, &params[i]);
    }
    ttapi_Run();
}

} // namespace Util3D
} // namespace XRay
