#include "vol_fog_common.h"
#include "vol_fog_params.h"

Texture3D<float4> t_Lighting : register(t0);
RWTexture3D<float4> u_Accum : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= VOL_FOG_W || id.y >= VOL_FOG_H)
        return;

    float3 inscatt = 0;
    float trans = 1.0;
    for (uint z = 0; z < VOL_FOG_D; ++z) {
        float z0 = VolFogFroxelZ(z, VOL_FOG_D, g_ZNear, g_ZFar);
        float z1 = VolFogFroxelZ(min(z + 1, VOL_FOG_D - 1), VOL_FOG_D, g_ZNear, g_ZFar);
        float dz = max(abs(z1 - z0), 0.05);
        float4 slice = t_Lighting[uint3(id.xy, z)];
        float sigma = slice.a * 0.85;
        float sliceT = exp(-sigma * dz);
        float3 light = slice.rgb / max(slice.a, 1e-5);
        inscatt += trans * light * (1.0 - sliceT);
        trans *= sliceT;
        u_Accum[uint3(id.xy, z)] = float4(min(inscatt, 2.5), trans);
    }
}
