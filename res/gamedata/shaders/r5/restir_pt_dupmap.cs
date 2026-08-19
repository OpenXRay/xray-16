#include "common.h"
#include "rt_common.h"
#include "restir_gi_common.h"
#include "restir_pt_common.h"

cbuffer ReSTIRPTDup : register(b5) {
    float2 g_ScreenSize;
    uint g_Pad0;
    uint g_Pad1;
};

Texture2D<uint4> t_PTB : register(t0);
RWTexture2D<float> u_Dup : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint2 pixel = id.xy;
    uint w = (uint)g_ScreenSize.x;
    uint h = (uint)g_ScreenSize.y;
    if (pixel.x >= w || pixel.y >= h)
        return;

    uint seed = t_PTB[pixel].z;
    if (seed == 0) {
        u_Dup[pixel] = 0;
        return;
    }
    uint same = 0;
    uint total = 0;
    [unroll]
    for (int y = -8; y <= 8; y += 2) {
        [unroll]
        for (int x = -8; x <= 8; x += 2) {
            int2 p = int2(pixel) + int2(x, y);
            if (p.x < 0 || p.y < 0 || p.x >= (int)w || p.y >= (int)h)
                continue;
            total++;
            if (t_PTB[p].z == seed)
                same++;
        }
    }
    u_Dup[pixel] = total > 1 ? saturate(((float)same - 1.0) / (float)(total - 1)) : 0;
}
