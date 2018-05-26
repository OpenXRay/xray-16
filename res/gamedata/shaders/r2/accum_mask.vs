#include "common.h"

float4 	main	( float4 P:	POSITION )	: POSITION
{
	return	mul	(m_WVP,	P	);
}
FXVS;
