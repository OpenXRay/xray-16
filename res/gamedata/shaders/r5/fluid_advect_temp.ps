#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    if( IsNonEmptyCell(input.texcoords.xyz) )
        return 0;

    float3 npos = GetAdvectedPosTexCoords(input);
    float4 ret = Texture_color.SampleLevel( samLinear, npos, 0) * modulate - k;
    ret = clamp(ret,float4(0,0,0,0),float4(5,5,5,5));
    return ret; 
}