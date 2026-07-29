#define SM_6_0
#include "common.h"

cbuffer LocalShadowEsmCB : register(b5)
{
    uint g_page;
    uint g_x0;
    uint g_y0;
    uint g_size;
    float g_k;
    float g_pad0;
    float g_pad1;
    float g_pad2;
};

Texture2DArray<float> g_DepthAtlas : register(t0);
RWTexture2DArray<float> g_EsmAtlas : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_size || id.y >= g_size)
        return;

    uint2 coord = uint2(g_x0 + id.x, g_y0 + id.y);
    float depth = g_DepthAtlas.Load(int4(coord, g_page, 0));
    g_EsmAtlas[uint3(coord, g_page)] = exp(g_k * saturate(depth));
}
