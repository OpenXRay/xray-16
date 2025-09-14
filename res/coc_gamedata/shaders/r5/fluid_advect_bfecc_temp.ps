#include "fluid_common.h"

#define Texture_phi_n Texture_color
#define Texture_phi_n_hat Texture_tempscalar

//	Advect MCCormack
//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    if( IsNonEmptyCell(input.texcoords.xyz) )
        return 0;

    // get advected new position
    float3 npos = input.cell0 - timestep * forward *
        Texture_velocity0.SampleLevel( samPointClamp, input.texcoords, 0 ).xyz;
    
    // convert new position to texture coordinates
    float3 nposTC = float3(npos.x/textureWidth, npos.y/textureHeight, (npos.z+0.5)/textureDepth);

    // find the texel corner closest to the semi-Lagrangian "particle"
    float3 nposTexel = floor( npos + float3( 0.5f, 0.5f, 0.5f ) );
    float3 nposTexelTC = float3( nposTexel.x/textureWidth, nposTexel.y/textureHeight, (nposTexel.z+0.5)/textureDepth);

    // ht (float-texel)
    float3 ht = float3(0.5f/textureWidth, 0.5f/textureHeight, 0.5f/textureDepth);

    // get the values of nodes that contribute to the interpolated value
    // (texel centers are at float-integer locations)
    float4 nodeValues[8];
    nodeValues[0] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3(-ht.x, -ht.y, -ht.z), 0 );
    nodeValues[1] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3(-ht.x, -ht.y,  ht.z), 0 );
    nodeValues[2] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3(-ht.x,  ht.y, -ht.z), 0 );
    nodeValues[3] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3(-ht.x,  ht.y,  ht.z), 0 );
    nodeValues[4] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3( ht.x, -ht.y, -ht.z), 0 );
    nodeValues[5] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3( ht.x, -ht.y,  ht.z), 0 );
    nodeValues[6] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3( ht.x,  ht.y, -ht.z), 0 );
    nodeValues[7] = Texture_phi_n.SampleLevel( samPointClamp, nposTexelTC + float3( ht.x,  ht.y,  ht.z), 0 );

    // determine a valid range for the result
    float4 phiMin = min(min(min(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
    phiMin = min(min(min(min(phiMin, nodeValues [4]), nodeValues [5]), nodeValues [6]), nodeValues [7]);
    
    float4 phiMax = max(max(max(nodeValues[0], nodeValues [1]), nodeValues [2]), nodeValues [3]);
    phiMax = max(max(max(max(phiMax, nodeValues [4]), nodeValues [5]), nodeValues [6]), nodeValues [7]);

    float4 r;
    // Perform final MACCORMACK advection step:
    // You can use point sampling and keep Texture_phi_n_1_hat
    //  r = Texture_phi_n_1_hat.SampleLevel( samPointClamp, input.texcoords, 0 )
    // OR use bilerp to avoid the need to keep a separate texture for phi_n_1_hat
    r = Texture_phi_n.SampleLevel( samLinear, nposTC, 0)
        + 0.5 * ( Texture_phi_n.SampleLevel( samPointClamp, input.texcoords, 0 ) -
                 Texture_phi_n_hat.SampleLevel( samPointClamp, input.texcoords, 0 ) );

    // clamp result to the desired range
    r = max( min( r, phiMax ), phiMin );

	float4 ret = r*modulate - k;
	ret = clamp(ret,float4(0,0,0,0),float4(5,5,5,5));
	return ret;
}