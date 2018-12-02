#include "stdafx.h"
#include "MathUtil.hpp"

#ifdef _EDITOR
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#else
#include "xrCore/Animation/Bone.hpp"
#include "Layers/xrRender/SkeletonXVertRender.h"
#define RENDER 1
#include "xrEngine/Render.h"
#include "Layers/xrRender/light.h"
#endif
#include "PLC_SSE.hpp"
#if defined(WINDOWS) && defined(XR_X86)
#include "SkinXW_SSE.hpp"
#else
#include "SkinXW_CPP.hpp"
#include "PLC_CPP.hpp"
#endif

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
    if (initialized)
        return;
#if defined(WINDOWS) && defined(XR_X86)
    Skin1W = Skin1W_SSE;
    Skin2W = Skin2W_SSE;
    Skin3W = Skin3W_SSE;
    Skin4W = Skin4W_SSE;
    PLCCalc = PLCCalc_SSE;
#else
    Skin1W = Skin1W_CPP;
    Skin2W = Skin2W_CPP;
    Skin3W = Skin3W_CPP;
    Skin4W = Skin4W_CPP;
    PLCCalc = PLCCalc_SSE;
    //PLCCalc = PLCCalc_CPP;
#endif
    // XXX: use PLC_energy and iCeil too
    // SSE implementations of this functions is not used.
    // Found duplicate implementation in src\Layers\xrRenderPC_R1\LightShadows.cpp
    // Search for other duplicates

    initialized = true;
}
} // namespace Math
} // namespace XRay
