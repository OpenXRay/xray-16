#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    float4 fieldL = Texture_velocity1.SampleLevel( samPointClamp, LEFTCELL, 0 );
    float4 fieldR = Texture_velocity1.SampleLevel( samPointClamp, RIGHTCELL, 0 );
    float4 fieldB = Texture_velocity1.SampleLevel( samPointClamp, BOTTOMCELL, 0 );
    float4 fieldT = Texture_velocity1.SampleLevel( samPointClamp, TOPCELL, 0 );
    float4 fieldD = Texture_velocity1.SampleLevel( samPointClamp, DOWNCELL, 0 );
    float4 fieldU = Texture_velocity1.SampleLevel( samPointClamp, UPCELL, 0 );

    if( IsBoundaryCell(LEFTCELL) )  fieldL = GetObstVelocity(LEFTCELL);
    if( IsBoundaryCell(RIGHTCELL) ) fieldR = GetObstVelocity(RIGHTCELL);
    if( IsBoundaryCell(BOTTOMCELL) )fieldB = GetObstVelocity(BOTTOMCELL);
    if( IsBoundaryCell(TOPCELL) )   fieldT = GetObstVelocity(TOPCELL);
    if( IsBoundaryCell(DOWNCELL) )  fieldD = GetObstVelocity(DOWNCELL);
    if( IsBoundaryCell(UPCELL) )    fieldU = GetObstVelocity(UPCELL);

    float divergence =  0.5 *
        ( ( fieldR.x - fieldL.x ) + ( fieldT.y - fieldB.y ) + ( fieldU.z - fieldD.z ) );

    return divergence;
}