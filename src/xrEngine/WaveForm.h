#pragma once
#include <math.h>
#include "xrCore/xr_types.h"
#include "xrCore/math_constants.h"
#include "xrCommon/math_funcs_inline.h"

#pragma pack(push, 4)
struct WaveForm
{
    enum EFunction
    {
        fCONSTANT = 0,
        fSIN,
        fTRIANGLE,
        fSQUARE,
        fSAWTOOTH,
        fINVSAWTOOTH,
        fFORCE32 = u32(-1)
    };

    IC float signf(float t) noexcept { return t / _abs(t); }
    IC float Func(float t) noexcept
    {
        switch (F)
        {
        case fCONSTANT: return 0;
        case fSIN: return _sin(t * PI_MUL_2);
        case fTRIANGLE: return asinf(_sin((t - 0.25f) * PI_MUL_2)) / PI_DIV_2;
        case fSQUARE: return signf(_cos(t * PI));
        case fSAWTOOTH: return atanf(tanf((t + 0.5f) * PI)) / PI_DIV_2;
        case fINVSAWTOOTH: return -(atanf(tanf((t + 0.5f) * PI)) / PI_DIV_2);
        default: return 0.f;
        }
    }

public:
    EFunction F{ fCONSTANT };
    float arg[4] = { 0.0f, 1.0f, 0.0f, 1.0f };

    IC float Calculate(float t) noexcept
    {
        // y = arg0 + arg1*func( (time+arg2)*arg3 )
        float x = (t + arg[2]) * arg[3];
        return arg[0] + arg[1] * Func(x - floorf(x));
    }

    IC bool Similar(const WaveForm& W) const noexcept
    {
        if (!fsimilar(arg[0], W.arg[0], EPS_L))
            return false;
        if (!fsimilar(arg[1], W.arg[1], EPS_L))
            return false;
        if (fis_zero(arg[1], EPS_L))
            return true;
        if (F != W.F)
            return false;
        if (!fsimilar(arg[2], W.arg[2], EPS_L))
            return false;
        if (!fsimilar(arg[3], W.arg[3], EPS_L))
            return false;
        return true;
    }
};
#pragma pack(pop)
