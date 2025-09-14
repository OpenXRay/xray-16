#include "fluid_common.h"

struct PSDrawBoxOut
{
    float4 obstacle : SV_TARGET0;
    float4 velocity : SV_TARGET1;
};

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
PSDrawBoxOut main( p_fluidsim_dyn_aabb input )
{
    PSDrawBoxOut voxel;

    // cells completely inside box = 0.5
	if( (input.clip0.x>BOX_EXPANSION) && (input.clip1.x>BOX_EXPANSION) && 
		(input.clip0.y>BOX_EXPANSION) && (input.clip1.y>BOX_EXPANSION) &&
		(input.clip0.z>BOX_EXPANSION) && (input.clip1.z>BOX_EXPANSION) )
    {
        voxel.obstacle = 0.5;
        voxel.velocity = 0;
        return voxel;
    }
	else	// cells in box boundary = 1.0
    {
        voxel.obstacle = 1.0;
        voxel.velocity = float4(input.velocity,1);
        return voxel;
    }
}