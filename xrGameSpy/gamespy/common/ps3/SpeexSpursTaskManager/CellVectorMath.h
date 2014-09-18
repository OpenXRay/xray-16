#ifndef __CELL_VECTORMATH_H__
#define __CELL_VECTORMATH_H__

#ifndef __CELLOS_LV2__
class vec_float4
{
	float x, y, z, w;
} 
#ifdef __GNUC__
__attribute__ ((aligned (16)));
#else
__declspec(align(16));
#endif // __GNUC__
#endif // __CELLOS_LV2__

#ifdef __CELLOS_LV2__
#include "vectormath_soa.h"
#else
#include "vectormath_scalar/vectormath_aos.h"
#endif // __CELLOS_LV2__

using namespace Vectormath;
using namespace Vectormath::Aos;

#endif /* __CELL_VECTORMATH_H__ */
