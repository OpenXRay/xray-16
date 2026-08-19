#include "vol_fog_common.h"
#include "vol_fog_params.h"

RWTexture3D<float4> u_Density : register(u0);

[numthreads(8, 8, 4)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= VOL_FOG_W || id.y >= VOL_FOG_H || id.z >= VOL_FOG_D)
        return;

    float3 worldPos = VolFogFroxelWorldPos(id, g_InvViewProj, g_CameraPos.xyz, g_ZNear, g_ZFar);
    float heightTerm = exp(-g_FogTune.y * max(worldPos.y - g_FogTune.x, 0.0));
    float cloud = VolFogCloudMask(worldPos, g_FogTune2.y, g_FogTune.w);
    float density = max(g_FogTune.z * heightTerm * cloud, 0.0);
    float3 albedo = saturate(g_FogColor.rgb);
    u_Density[id] = float4(albedo, density);
}
