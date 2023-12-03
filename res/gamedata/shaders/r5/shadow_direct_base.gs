#include "common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Geometry
#define CASCADE_COUNT 3
uniform float4x4 m_shadow_direct[CASCADE_COUNT]; // световая матрица (fuckingsun->X.D.combine)

struct geometry_output
{
	float4 hpos 	: SV_Position;
	uint RTIndex 	: SV_RenderTargetArrayIndex;
};

[maxvertexcount(CASCADE_COUNT*3)]
void main(triangle v2p_shadow_direct input[3], inout TriangleStream<geometry_output> ShadowStream)
{
	[unroll]
    for( int f = 0; f < CASCADE_COUNT; ++f )
    {
		{
	        geometry_output output = (geometry_output)0;

			output.RTIndex = f;

			[unroll]
			for( uint v = 0; v < 3; ++v )
			{
				float4 pos_w = input[v].position_w; // мировая позиция
				//float4 light_wvp = mul(pos_w, m_shadow_direct[f+3]);
				float4 light_wvp = mul(m_shadow_direct[f], pos_w);
				//output.hpos = mul(light_wvp, m_cubemap_shadow_project);
				output.hpos = light_wvp;

				ShadowStream.Append( output );
			}

			ShadowStream.RestartStrip();
        }
    }
}
