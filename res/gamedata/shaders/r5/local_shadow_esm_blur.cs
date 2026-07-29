#define SM_6_0
#include "common.h"

cbuffer LocalShadowEsmBlurCB : register(b5)
{
    uint g_page;
    uint g_x0;
    uint g_y0;
    uint g_size;
    uint g_horizontal;
    uint g_pad0;
    uint g_pad1;
    uint g_pad2;
};

Texture2DArray<float> g_Input : register(t0);
RWTexture2DArray<float> g_Output : register(u0);

static const float kW[5] = { 0.06136, 0.24477, 0.38774, 0.24477, 0.06136 };

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_size || id.y >= g_size)
        return;

    int2 base = int2(g_x0 + id.x, g_y0 + id.y);
    float acc = 0.0;
    [unroll] for (int i = -2; i <= 2; ++i)
    {
        int2 p = base;
        if (g_horizontal != 0)
            p.x = clamp(base.x + i, int(g_x0), int(g_x0 + g_size - 1));
        else
            p.y = clamp(base.y + i, int(g_y0), int(g_y0 + g_size - 1));
        acc += g_Input.Load(int4(p, g_page, 0)) * kW[i + 2];
    }
    g_Output[uint3(base, g_page)] = acc;
}
