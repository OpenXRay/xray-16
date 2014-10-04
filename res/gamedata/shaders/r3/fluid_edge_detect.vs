#include "fluid_common_render.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Vertex
//	A full-screen edge detection pass to locate artifacts
VS_OUTPUT_EDGE main( VS_INPUT input )
{
    VS_OUTPUT_EDGE output = (VS_OUTPUT_EDGE)0;
    output.position = float4(input.pos,1);

    float2 texelSize = 1.0 / float2(RTWidth,RTHeight);
    float2 center = float2( (input.pos.x+1)/2.0 , 1.0 - (input.pos.y+1)/2.0 );

    // Eight nearest neighbours needed for Sobel.
    output.textureUV00 = center + float2(-texelSize.x, -texelSize.y);
    output.textureUV01 = center + float2(-texelSize.x,  0);
    output.textureUV02 = center + float2(-texelSize.x,  texelSize.y);

    output.textureUV10 = center + float2(0, -texelSize.y);
    output.textureUV12 = center + float2(0,  texelSize.y);

    output.textureUV20 = center + float2(texelSize.x, -texelSize.y);
    output.textureUV21 = center + float2(texelSize.x,  0);
    output.textureUV22 = center + float2(texelSize.x,  texelSize.y);

    return output;
}