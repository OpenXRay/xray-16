#include "fluid_common_render.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Vertex
//	TODO: DX10: replace WorldViewProjection with m_WVP
PS_INPUT_RAYDATA_BACK main(VS_INPUT input)
{
    PS_INPUT_RAYDATA_BACK output = (PS_INPUT_RAYDATA_BACK)0;
    //output.pos = mul(float4(input.pos,1), WorldViewProjection);
	output.pos = mul(WorldViewProjection, float4(input.pos,1));
    output.depth = output.pos.w;
    return output;
}