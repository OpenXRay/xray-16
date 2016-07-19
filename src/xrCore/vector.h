// XXX tamlin: This header is a HOG! It includes tons of headers, while it's the ONLY
// place defining f.ex. fsimilar, deg2rad, clampr. Split it up!
#pragma once
#ifndef _vector_included
#define _vector_included

#include "math_constants.h"
// Define types and namespaces (CPU & FPU)
#include "_types.h"
#include "_math.h"
#include "_bitwise.h"
#include "_std_extensions.h"

// comparisions
IC BOOL fsimilar(float a, float b, float cmp = EPS) { return _abs(a - b) < cmp; }
IC BOOL dsimilar(double a, double b, double cmp = EPS) { return _abs(a - b) < cmp; }
IC BOOL fis_zero(float val, float cmp = EPS_S) { return _abs(val) < cmp; }
IC BOOL dis_zero(double val, double cmp = EPS_S) { return _abs(val) < cmp; }
// degree 2 radians and vice-versa
namespace implement
{
template <class T>
ICF T deg2rad(T val)
{
    return (val * T(M_PI) / T(180));
};
template <class T>
ICF T rad2deg(T val)
{
    return (val * T(180) / T(M_PI));
};
};
ICF float deg2rad(float val) { return implement::deg2rad(val); }
ICF double deg2rad(double val) { return implement::deg2rad(val); }
ICF float rad2deg(float val) { return implement::rad2deg(val); }
ICF double rad2deg(double val) { return implement::rad2deg(val); }
// clamping/snapping
template <class T>
IC void clamp(T& val, const T& _low, const T& _high)
{
    if (val < _low)
        val = _low;
    else if (val > _high)
        val = _high;
};
template <class T>
IC T clampr(const T& val, const T& _low, const T& _high)
{
    if (val < _low)
        return _low;
    else if (val > _high)
        return _high;
    else
        return val;
};
IC float snapto(float value, float snap)
{
    if (snap <= 0.f)
        return value;
    return float(iFloor((value + (snap * 0.5f)) / snap)) * snap;
};

template <class T>
struct _quaternion;

#pragma pack(push)
#pragma pack(1)

#include "_random.h"

#include "_color.h"
#include "_vector3d.h"
#include "_vector2.h"
#include "_vector4.h"
#include "_matrix.h"
#include "_matrix33.h"
#include "_quaternion.h"
#include "_rect.h"
#include "_fbox.h"
#include "_fbox2.h"
#include "_obb.h"
#include "_sphere.h"
#include "_cylinder.h"
#include "_random.h"
#include "_compressed_normal.h"
#include "_plane.h"
#include "_plane2.h"
#include "_flags.h"
#ifdef DEBUG
#include "dump_string.h"
#endif
#pragma pack(pop)

// normalize angle (0..2PI)
float angle_normalize_always(float a);


// normalize angle (0..2PI)
float angle_normalize(float a);


// -PI .. +PI
float angle_normalize_signed(float a);



// -PI..PI
float angle_difference_signed(float a, float b);

// 0..PI
float angle_difference(float a, float b);

bool are_ordered(float const value0, float const value1, float const value2);

bool is_between(float const value, float const left, float const right);
// c=current, t=target, s=speed, dt=dt
bool angle_lerp(float& c, float t, float s, float dt);


// Just lerp :) expects normalized angles in range [0..2PI)
float angle_lerp(float A, float B, float f);

float angle_inertion(float src, float tgt, float speed, float clmp, float dt);

float angle_inertion_var(float src, float tgt, float min_speed, float max_speed, float clmp, float dt);

#endif
