#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    if( IsBoundaryCell(input.texcoords.xyz) )
        return GetObstVelocity(input.texcoords.xyz);

    float pCenter = Texture_pressure.SampleLevel( samPointClamp, input.texcoords, 0 ); 
    float pL = Texture_pressure.SampleLevel( samPointClamp, LEFTCELL, 0 );
    float pR = Texture_pressure.SampleLevel( samPointClamp, RIGHTCELL, 0 );
    float pB = Texture_pressure.SampleLevel( samPointClamp, BOTTOMCELL, 0 );
    float pT = Texture_pressure.SampleLevel( samPointClamp, TOPCELL, 0 );
    float pD = Texture_pressure.SampleLevel( samPointClamp, DOWNCELL, 0 );
    float pU = Texture_pressure.SampleLevel( samPointClamp, UPCELL, 0 );

    float4 velocity;
    float3 obstV = float3(0,0,0);
    float3 vMask = float3(1,1,1);
    float3 vLeft = GetObstVelocity(LEFTCELL);
    float3 vRight = GetObstVelocity(RIGHTCELL);
    float3 vBottom = GetObstVelocity(BOTTOMCELL);
    float3 vTop = GetObstVelocity(TOPCELL);
    float3 vDown = GetObstVelocity(DOWNCELL);
    float3 vUp = GetObstVelocity(UPCELL);
    float3 v;

    if( IsBoundaryCell(LEFTCELL) )  { pL = pCenter; obstV.x = vLeft.x; vMask.x = 0; }
    if( IsBoundaryCell(RIGHTCELL) ) { pR = pCenter; obstV.x = vRight.x; vMask.x = 0; }
    if( IsBoundaryCell(BOTTOMCELL) ){ pB = pCenter; obstV.y = vBottom.y; vMask.y = 0; }
    if( IsBoundaryCell(TOPCELL) )   { pT = pCenter; obstV.y = vTop.y; vMask.y = 0; }
    if( IsBoundaryCell(DOWNCELL) )  { pD = pCenter; obstV.z = vDown.z; vMask.z = 0; }
    if( IsBoundaryCell(UPCELL) )    { pU = pCenter; obstV.z = vUp.z; vMask.z = 0; }

    v = ( Texture_velocity1.SampleLevel( samPointClamp, input.texcoords, 0 ).xyz -
                 (0.5*modulate*float3( pR - pL, pT - pB, pU - pD )) );

    velocity.xyz = (vMask * v) + obstV;

    return velocity;
}