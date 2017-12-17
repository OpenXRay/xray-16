#pragma once

// Undef some macros
#ifdef M_PI
#undef M_PI
#endif

#ifdef PI
#undef PI
#endif

// Constants
// Epsilon's, to use as difference limit when comparing float's.
constexpr float EPS_S = 0.0000001f;
constexpr float EPS   = 0.0000100f;
constexpr float EPS_L = 0.0010000f;

#ifdef M_SQRT1_2
#undef M_SQRT1_2
#endif
// XXX: The following section contains constants with a _tad_ many decimals to fit in a float. :-)
//      They used to be macro's, so they could be used as double too.
constexpr float M_SQRT1_2 = 0.7071067811865475244008443621048f;//490;

constexpr float M_PI = 3.1415926535897932384626433832795f;
constexpr float PI = 3.1415926535897932384626433832795f;
constexpr float PI_MUL_2 = 6.2831853071795864769252867665590f;
constexpr float PI_MUL_3 = 9.4247779607693797153879301498385f;
constexpr float PI_MUL_4 = 12.566370614359172953850573533118f;
constexpr float PI_MUL_6 = 18.849555921538759430775860299677f;
constexpr float PI_MUL_8 = 25.132741228718345907701147066236f;
constexpr float PI_DIV_2 = 1.5707963267948966192313216916398f;
constexpr float PI_DIV_3 = 1.0471975511965977461542144610932f;
constexpr float PI_DIV_4 = 0.7853981633974483096156608458199f;
constexpr float PI_DIV_6 = 0.5235987755982988730771072305466f;
constexpr float PI_DIV_8 = 0.3926990816987241548078304229099f;
