#pragma once
#ifndef _vector_included
#define _vector_included

// Undef some macros
#ifdef M_PI
#undef M_PI
#endif

#ifdef PI
#undef PI
#endif

// Select platform
#ifdef _MSC_VER
#define M_VISUAL
#endif
#ifdef __BORLANDC__
#define M_BORLAND
#endif

// Constants
#ifdef M_VISUAL
const float EPS_S = 0.0000001f;
const float EPS = 0.0000100f;
const float EPS_L = 0.0010000f;

#undef M_SQRT1_2
const float M_SQRT1_2 = 0.7071067811865475244008443621048f; // 490;

const float M_PI = 3.1415926535897932384626433832795f;
const float PI = 3.1415926535897932384626433832795f;
const float PI_MUL_2 = 6.2831853071795864769252867665590f;
const float PI_MUL_3 = 9.4247779607693797153879301498385f;
const float PI_MUL_4 = 12.566370614359172953850573533118f;
const float PI_MUL_6 = 18.849555921538759430775860299677f;
const float PI_MUL_8 = 25.132741228718345907701147066236f;
const float PI_DIV_2 = 1.5707963267948966192313216916398f;
const float PI_DIV_3 = 1.0471975511965977461542144610932f;
const float PI_DIV_4 = 0.7853981633974483096156608458199f;
const float PI_DIV_6 = 0.5235987755982988730771072305466f;
const float PI_DIV_8 = 0.3926990816987241548078304229099f;

#endif
#ifdef M_BORLAND
#define EPS_S 0.0000001f
#define EPS 0.0000100f
#define EPS_L 0.0010000f

#define M_PI 3.1415926535897932384626433832795f
#define PI 3.1415926535897932384626433832795f
#define PI_MUL_2 6.2831853071795864769252867665590f
#define PI_MUL_3 9.4247779607693797153879301498385f
#define PI_MUL_4 12.566370614359172953850573533118f
#define PI_MUL_6 18.849555921538759430775860299677f
#define PI_MUL_8 25.132741228718345907701147066236f
#define PI_DIV_2 1.5707963267948966192313216916398f
#define PI_DIV_3 1.0471975511965977461542144610932f
#define PI_DIV_4 0.7853981633974483096156608458199f
#define PI_DIV_6 0.5235987755982988730771072305466f
#define PI_DIV_8 0.3926990816987241548078304229099f
#endif

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

// pre-definitions
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
