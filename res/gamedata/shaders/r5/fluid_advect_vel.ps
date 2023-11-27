#include "fluid_common.h"

float GravityBuoyancy;

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    float3 npos = GetAdvectedPosTexCoords(input);

	float4 Velocity = Texture_velocity0.SampleLevel( samLinear, npos, 0) * modulate;

#ifdef	USE_GRAVITY

	float Dencity = Texture_color.SampleLevel( samLinear, npos, 0);
	Velocity.y += Dencity * GravityBuoyancy;

#endif	//	USE_GRAVITY

	return Velocity;
}