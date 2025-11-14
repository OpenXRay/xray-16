#include "fluid_common_render.h"

PS_INPUT_RAYCAST main (VS_INPUT input)
{
    PS_INPUT_RAYCAST output = (PS_INPUT_RAYCAST)0;
    output.pos = float4(input.pos,1);
	output.posInGrid = mul( InvWorldViewProjection, float4( input.pos.xy*ZNear, 0, ZNear ));
    return output;
}