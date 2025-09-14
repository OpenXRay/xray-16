#include "fluid_common_render.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
//	TODO: DX10: replace WorldViewProjection with m_WVP
float4 main(PS_INPUT_RAYCAST input) : SV_Target
{
    float edge = edgeTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight)).r;

#ifdef	RENDER_FIRE
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
	if(edge > 0 && tex.a > 0)
		return Raycast(input);
	else
		return tex;
#else	//	RENDER_FIRE
    float4 tex = rayCastTex.Sample(samLinearClamp, float2(input.pos.x/RTWidth,input.pos.y/RTHeight));
	if(edge > 0 && tex.a > 0)
		return Raycast(input)*DiffuseLight;
		//return float4(1,0,0,1);
	else
		return tex*DiffuseLight;
#endif	//	RENDER_FIRE
}