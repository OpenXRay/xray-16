#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    float pCenter = Texture_pressure.SampleLevel( samPointClamp, input.texcoords, 0 );
    // Texture_tempvector contains the "divergence" computed by PS_DIVERGENCE
    float bC = Texture_tempvector.SampleLevel( samPointClamp, input.texcoords, 0 );

    float pL = Texture_pressure.SampleLevel( samPointClamp, LEFTCELL, 0 );
    float pR = Texture_pressure.SampleLevel( samPointClamp, RIGHTCELL, 0 );
    float pB = Texture_pressure.SampleLevel( samPointClamp, BOTTOMCELL, 0 );
    float pT = Texture_pressure.SampleLevel( samPointClamp, TOPCELL, 0 );
    float pD = Texture_pressure.SampleLevel( samPointClamp, DOWNCELL, 0 );
    float pU = Texture_pressure.SampleLevel( samPointClamp, UPCELL, 0 );

    if( IsBoundaryCell(LEFTCELL) )  pL = pCenter;
    if( IsBoundaryCell(RIGHTCELL) ) pR = pCenter;
    if( IsBoundaryCell(BOTTOMCELL) )pB = pCenter;
    if( IsBoundaryCell(TOPCELL) )   pT = pCenter; 
    if( IsBoundaryCell(DOWNCELL) )  pD = pCenter;  
    if( IsBoundaryCell(UPCELL) )    pU = pCenter;

    return( pL + pR + pB + pT + pU + pD - bC ) /6.0;
}