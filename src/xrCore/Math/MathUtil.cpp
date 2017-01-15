#include "MathUtil.hpp"
#include "Threading/ttapi.h"
#include "stdafx.h"

#ifdef _EDITOR
#include "SkeletonCustom.h"
#include "SkeletonX.h"
#else
#include "Layers/xrRender/SkeletonXVertRender.h"
#include "xrCore/Animation/Bone.hpp"
#define RENDER 1
#include "Layers/xrRender/light.h"
#include "xrEngine/Render.h"
#endif

#include "PLC_SSE.hpp"
#include "Skin4W_MT.hpp"
#include "SkinXW_SSE.hpp"
#include "_math.h"

namespace XRay
{
namespace Math
{
Skin1WFunc Skin1W;
Skin2WFunc Skin2W;
Skin3WFunc Skin3W;
Skin4WFunc Skin4W;
PLCCalcFunc PLCCalc;

void Initialize()
{
    static bool initialized = false;
    if (initialized) return;
    Skin1W = Skin1W_SSE;
    Skin2W = Skin2W_SSE;
    Skin3W = Skin3W_SSE;
    Skin4W = Skin4W_SSE;
    Skin4W_MTs = Skin4W_SSE;
    PLCCalc = PLCCalc_SSE;
    if (ttapi_GetWorkerCount() > 1) Skin4W = Skin4W_MT;
    initialized = true;
}

}  // namespace Math
}  // namespace XRay
