// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_MATH_H
#define NV_MATH_H

#include <nvcore/nvcore.h>
#include <nvcore/Debug.h>

#include <math.h>

// Function linkage
#if NVMATH_SHARED
#ifdef NVMATH_EXPORTS
#define NVMATH_API DLL_EXPORT
#define NVMATH_CLASS DLL_EXPORT_CLASS
#else
#define NVMATH_API DLL_IMPORT
#define NVMATH_CLASS DLL_IMPORT
#endif
#else // NVMATH_SHARED
#define NVMATH_API
#define NVMATH_CLASS
#endif // NVMATH_SHARED

#ifndef PI
#define PI      			float(3.1415926535897932384626433833)
#endif

#define NV_EPSILON			(0.0001f)
#define NV_NORMAL_EPSILON	(0.001f)

/*
#define SQ(r)				((r)*(r))

#define	SIGN_BITMASK		0x80000000

/// Integer representation of a floating-point value.
#define IR(x)					((uint32 &)(x))

/// Absolute integer representation of a floating-point value
#define AIR(x)					(IR(x) & 0x7fffffff)

/// Floating-point representation of an integer value.
#define FR(x)					((float&)(x))

/// Integer-based comparison of a floating point value.
/// Don't use it blindly, it can be faster or slower than the FPU comparison, depends on the context.
#define IS_NEGATIVE_FLOAT(x)	(IR(x)&SIGN_BITMASK)
*/

inline double sqrt_assert(const double f)
{
	nvDebugCheck(f >= 0.0f);
	return sqrt(f);
}

inline float sqrtf_assert(const float f)
{
	nvDebugCheck(f >= 0.0f);
	return sqrtf(f);
}

inline double acos_assert(const double f)
{
	nvDebugCheck(f >= -1.0f && f <= 1.0f);
	return acos(f);
}

inline float acosf_assert(const float f)
{
	nvDebugCheck(f >= -1.0f && f <= 1.0f);
	return acosf(f);
}

inline double asin_assert(const double f)
{
	nvDebugCheck(f >= -1.0f && f <= 1.0f);
	return asin(f);
}

inline float asinf_assert(const float f)
{
	nvDebugCheck(f >= -1.0f && f <= 1.0f);
	return asinf(f);
}

// Replace default functions with asserting ones.
#if ! defined(_MSC_VER) || (defined(_MSC_VER) && (_MSC_VER<1700))
#define sqrt sqrt_assert
#define sqrtf sqrtf_assert
#define acos acos_assert
#define acosf acosf_assert
#define asin asin_assert
#define asinf asinf_assert
#endif

#if NV_OS_WIN32
#include <float.h>
#endif

namespace nv
{
inline float toRadian(float degree) { return degree * (PI / 180.0f); }
inline float toDegree(float radian) { return radian * (180.0f / PI); }
	
inline bool equal(const float f0, const float f1, const float epsilon = NV_EPSILON)
{
	return fabs(f0-f1) <= epsilon;
}

inline bool isZero(const float f, const float epsilon = NV_EPSILON)
{
	return fabs(f) <= epsilon;
}

inline bool isFinite(const float f)
{
#if NV_OS_WIN32 && !NV_CC_GNUC
	return _finite(f) != 0;
#elif NV_OS_DARWIN || NV_CC_GNUC
	return isfinite(f);
#elif NV_OS_LINUX
	return finitef(f);
#else
#	error "isFinite not supported"
#endif
//return std::isfinite (f);
//return finite (f);
}

inline bool isNan(const float f)
{
#if NV_OS_WIN32 && !NV_CC_GNUC
	return _isnan(f) != 0;
#elif NV_OS_DARWIN || NV_CC_GNUC
	return isnan(f);
#elif NV_OS_LINUX
	return isnanf(f);
#else
#	error "isNan not supported"
#endif
}

inline uint log2(uint i)
{
	uint value = 0;
	while( i >>= 1 ) {
		value++;
	}
	return value;
}

inline float lerp(float f0, float f1, float t)
{
	const float s = 1.0f - t;
	return f0 * s + f1 * t;
}

inline float square(float f)
{
	return f * f;
}

} // nv

#endif // NV_MATH_H
