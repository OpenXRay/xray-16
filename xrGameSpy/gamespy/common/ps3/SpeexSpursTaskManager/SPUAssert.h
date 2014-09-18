#ifndef __SPU_ASSERT_H__
#define __SPU_ASSERT_H__

// Author: Sauce
// 1/18/2006
// Better assert on SPU side, but it assumes spu_printf works.

#ifdef _DEBUG

#ifdef __CELLOS_LV2__
#include <spu_printf.h>
#define SPU_ASSERT(cond) do { if (__builtin_expect(!(cond), 0)) { spu_printf("SPU: Assertion failed!  Expression: " #cond "\n    in %s at " __FILE__ ":%i\n", __FUNCTION__, __LINE__); spu_hcmpeq((cond), 0); } } while (0)
#else // __CELLOS_LV2__
#define SPU_ASSERT(cond) assert(cond)
#endif //__CELLOS_LV2__

#else  // _DEBUG

#ifdef __CELLOS_LV2__
#include <spu_printf.h>
#define SPU_ASSERT(cond) do { if (__builtin_expect(!(cond), 0)) { spu_printf("SPU: Assertion failed!  Expression: " #cond "\n    in %s at " __FILE__ ":%i\n", __FUNCTION__, __LINE__); spu_hcmpeq((cond), 0); } } while (0)
// Sauce
// Later on we'll want no asserts in release builds
//#define SPU_ASSERT(cond) do {} while (0)
#else  // __CELLOS_LV2__
#define SPU_ASSERT(cond)  assert(cond)
#endif // __CELLOS_LV2__

#endif // _DEBUG

// Usage:
// SPU_COMPILE_TIME_ASSERT(sizeof(MyStructure) <= 128);
// Gives the following error message if it fails:
//  error: size of array `spu_compile_time_assert_failed' is negative
#define SPU_COMPILE_TIME_ASSERT(cond) extern char spu_compile_time_assert_failed[cond ? 1 : -1]

// Usage:
// SPU_NAMED_COMPILE_TIME_ASSERT(MyStructure_is_more_than_128_bytes, sizeof(MyStructure) <= 128);
// Gives the following error message if it fails:
//  error: size of array `MyStructure_is_more_than_128_bytes' is negative
#define SPU_NAMED_COMPILE_TIME_ASSERT(name, cond) extern char name[cond ? 1 : -1]


#endif
