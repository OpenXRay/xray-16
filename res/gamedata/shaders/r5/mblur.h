#ifndef	MBLUR_H
#define MBLUR_H

#ifndef 	USE_MBLUR
float3 	mblur	(float2 UV, float3 pos, float3 c_original)	{ return c_original; }
#else
#include "common.h"

float3 	mblur	(float2 UV, float3 pos, float3 c_original)	{ return c_original; }
#endif

#endif
