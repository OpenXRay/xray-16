#include "fluid_common.h"

struct PSDrawBoxOut
{
    float4 obstacle : SV_TARGET0;
    float4 velocity : SV_TARGET1;
};

bool PointIsInsideBox(float3 p, float3 LBUcorner, float3 RTDcorner)
{
    return ((p.x > LBUcorner.x) && (p.x < RTDcorner.x)
        &&  (p.y > LBUcorner.y) && (p.y < RTDcorner.y)
        &&  (p.z > LBUcorner.z) && (p.z < RTDcorner.z));
}

//////////////////////////////////////////////////////////////////////////////////////////
//	Pixel
PSDrawBoxOut main( p_fluidsim_clip input )
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
        //voxel.velocity = float4(obstVelocity.xyz,1);
		voxel.velocity = 0;
        return voxel;
    }
}