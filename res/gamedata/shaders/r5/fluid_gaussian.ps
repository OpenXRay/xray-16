#include "fluid_common.h"

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
float4 main( p_fluidsim input ) : SV_Target
{
    if( IsNonEmptyCell(input.texcoords.xyz) )
        return 0;

	float dist = length( input.cell0 - center ) * size;
//	float dist = length( input.cell0 - center ) * size / 2;
    float4 result;
//    result.rgb = splatColor;    // + sin(splatColor.rgb*10.0+cell*5.0)*0.2;
	result.rgb = splatColor + length(splatColor)*sin(splatColor.rgb*10.0+input.cell0*5.0)*0.2;
//	result.a = exp( -dist*dist/(0.05) );
//	result.a = exp( -dist*dist );
	result.a = exp( -dist*dist/0.5 );

    return result;
}