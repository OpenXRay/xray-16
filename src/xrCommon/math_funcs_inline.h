#pragma once
#include <math.h>
#include "xrCore/math_constants.h"
#include "xrCommon/inlining_macros.h"
#include "xrCore/_bitwise.h" // iFloor

inline float _abs(float x) { return fabsf(x); }
inline float _sqrt(float x) { return sqrtf(x); }
inline float _sin(float x) { return sinf(x); }
inline float _cos(float x) { return cosf(x); }
inline double _abs(double x) { return fabs(x); }
inline double _sqrt(double x) { return sqrt(x); }
inline double _sin(double x) { return sin(x); }
inline double _cos(double x) { return cos(x); }

// comparisions
inline bool fsimilar(float a, float b, float cmp = EPS) { return _abs(a-b)<cmp; }
inline bool dsimilar(double a, double b, double cmp = EPS) { return _abs(a-b)<cmp; }

inline bool fis_zero(float val, float cmp = EPS_S) { return _abs(val)<cmp; }
inline bool dis_zero(double val, double cmp = EPS_S) { return _abs(val)<cmp; }

// degree to radians and vice-versa
ICF float  deg2rad(float  val) { return val*M_PI / 180; }
ICF double deg2rad(double val) { return val*M_PI / 180; }
ICF float  rad2deg(float  val) { return val*180 / M_PI; }
ICF double rad2deg(double val) { return val*180 / M_PI;}

// clamping/snapping
template <class T>
IC void clamp(T& val, const T& _low, const T& _high)
{
    if (val<_low)
        val = _low;
    else if (val>_high)
        val = _high;
}

template <class T>
IC T clampr(const T& val, const T& _low, const T& _high)
{
    if (val<_low)
        return _low;
    if (val>_high)
        return _high;
    return val;
}

IC float snapto(float value, float snap)
{
    if (snap <= 0.f)
        return value;
    return float(iFloor((value + (snap*0.5f)) / snap)) * snap;
}
