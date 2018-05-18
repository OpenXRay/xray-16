#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    float4 L = Texture_velocity1.SampleLevel( samPointClamp, LEFTCELL, 0 );
    float4 R = Texture_velocity1.SampleLevel( samPointClamp, RIGHTCELL, 0 );
    float4 B = Texture_velocity1.SampleLevel( samPointClamp, BOTTOMCELL, 0 );
    float4 T = Texture_velocity1.SampleLevel( samPointClamp, TOPCELL, 0 );
    float4 D = Texture_velocity1.SampleLevel( samPointClamp, DOWNCELL, 0 );
    float4 U = Texture_velocity1.SampleLevel( samPointClamp, UPCELL, 0 );

    float4 vorticity;
    vorticity.xyz = 0.5 * float3( (( T.z - B.z ) - ( U.y - D.y )) ,
                                 (( U.x - D.x ) - ( R.z - L.z )) ,
                                 (( R.y - L.y ) - ( T.x - B.x )) );
                                 
    return vorticity;
}