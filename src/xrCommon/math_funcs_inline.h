#pragma once
#include <math.h>
#include "xrCore/math_constants.h"
#include "xrCore/_bitwise.h" // iFloor

inline float _abs(float x) noexcept { return fabsf(x); }
inline float _sqrt(float x) noexcept { return sqrtf(x); }
inline float _sin(float x) noexcept { return sinf(x); }
inline float _cos(float x) noexcept { return cosf(x); }
inline double _abs(double x) noexcept { return fabs(x); }
inline double _sqrt(double x) noexcept { return sqrt(x); }
inline double _sin(double x) noexcept { return sin(x); }
inline double _cos(double x) noexcept { return cos(x); }

// comparisions
inline bool fsimilar(float a, float b, float cmp = EPS) { return _abs(a-b)<cmp; }
inline bool dsimilar(double a, double b, double cmp = EPS) { return _abs(a-b)<cmp; }

inline bool fis_zero(float val, float cmp = EPS_S) noexcept { return _abs(val) < cmp; }
inline bool dis_zero(double val, double cmp = EPS_S) noexcept { return _abs(val) < cmp; }

// degree to radians and vice-versa
constexpr float  deg2rad(float  val) noexcept { return val*M_PI / 180; }
constexpr double deg2rad(double val) noexcept { return val*M_PI / 180; }
constexpr float  rad2deg(float  val) noexcept { return val*180 / M_PI; }
constexpr double rad2deg(double val) noexcept { return val*180 / M_PI;}

// clamping/snapping
template <class T>
constexpr void clamp(T& val, const T& _low, const T& _high) noexcept
{
    if (val<_low)
        val = _low;
    else if (val>_high)
        val = _high;
}

// XXX: Check usages and provide overloads for native types where arguments are NOT references.
template <class T>
constexpr T clampr(const T& val, const T& _low, const T& _high) noexcept
{
    if (val < _low)
        return _low;
    if (val > _high)
        return _high;
    return val;
}

inline float snapto(float value, float snap)
{
    if (snap <= 0.f)
        return value;
    return float(iFloor((value + (snap*0.5f)) / snap)) * snap;
}
