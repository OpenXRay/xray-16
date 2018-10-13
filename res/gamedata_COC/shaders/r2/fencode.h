#ifndef	FENCODE_H
#define FENCODE_H

#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
uniform float3		v_encodeZ01;
uniform float3		v_decodeZ01;
float3	encode_tcRG	( float  z )	{ return z*v_encodeZ01.xyz;	}
float2	encode_tcB	( float  z )	{ return z*v_encodeZ01.z;	}
//////////////////////////////////////////////////////////////////////////////////////////
// 0..1 encoding with 21 bit precision
static	const	float3	pe_scale	= {1.f,		128.f,		16384.f		};
static	const 	float3 	pe_unscale21 	= {2.f/1.f,	2.f/128.f,	2.f/16384.f	};
static	const 	float3 	pe_unscale24 	= {1.f/1.f,	1.f/256.f,	1.f/65536.f	};
float 	decode_float21	( float3 rgb)	{	return 	dot	( rgb,	pe_unscale21);	}
float 	decode_float24	( float3 rgb)	{	return 	dot	( rgb,	pe_unscale24);	}

#endif
