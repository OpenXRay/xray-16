#pragma once
#include "xrCore/xrCore.h"
#include "xrCore/_vector3d.h"

struct vertRender;
struct vertBoned1W;
struct vertBoned2W;
struct vertBoned3W;
struct vertBoned4W;
class CBoneInstance;
class light;

using Skin1WFunc = void(*)(vertRender *d, vertBoned1W *s, u32 vCount, CBoneInstance *bones);
using Skin2WFunc = void(*)(vertRender *d, vertBoned2W *s, u32 vCount, CBoneInstance *bones);
using Skin3WFunc = void(*)(vertRender *d, vertBoned3W *s, u32 vCount, CBoneInstance *bones);
using Skin4WFunc = void(*)(vertRender *d, vertBoned4W *s, u32 vCount, CBoneInstance *bones);

using PLCCalcFunc = void(*)(int &c0, int &c1, int &c2, const Fvector &camPos, const Fvector *ps, const Fvector &n,
    const light *l, float energy, const Fvector &obj);

namespace XRay
{
namespace Math
{
    extern XRCORE_API Skin1WFunc Skin1W;
    extern XRCORE_API Skin2WFunc Skin2W;
    extern XRCORE_API Skin3WFunc Skin3W;
    extern XRCORE_API Skin4WFunc Skin4W;
    extern XRCORE_API PLCCalcFunc PLCCalc;

    void XRCORE_API Initialize();
} // namespace Math
} // namespace XRay
