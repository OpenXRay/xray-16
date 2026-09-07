#define SM_5_0

cbuffer LocalShadowArgsParams : register(b5)
{
    uint4 g_Caps0;
    uint4 g_Caps1;
};

StructuredBuffer<uint> g_Stats : register(t14);
RWByteAddressBuffer g_Args : register(u0);

[numthreads(1, 1, 1)]
void main()
{
    uint mode = g_Caps1.z;
    if (mode == 1u)
    {
        g_Args.Store4(0, uint4(384u, g_Stats[36], 0u, 0u));
        g_Args.Store4(16, uint4(384u, g_Stats[37], 0u, 0u));
        g_Args.Store4(32, uint4(384u, g_Stats[38], 0u, 0u));
        return;
    }
    if (mode == 2u)
    {
        g_Args.Store4(48, uint4(384u, g_Stats[20], 0u, 0u));
        g_Args.Store4(64, uint4(384u, g_Stats[22], 0u, 0u));
        return;
    }
    if (mode == 3u)
    {
        g_Args.Store4(80, uint4(384u, g_Stats[28], 0u, 0u));
        return;
    }
    g_Args.Store4(0, uint4(384u, min(g_Stats[4], g_Caps0.x), 0u, 0u));
    g_Args.Store4(16, uint4(384u, min(g_Stats[5], g_Caps0.y), 0u, 0u));
    g_Args.Store4(32, uint4(384u, min(g_Stats[6], g_Caps0.z), 0u, 0u));
    g_Args.Store4(48, uint4(384u, min(g_Stats[20], g_Caps0.w), 0u, 0u));
    g_Args.Store4(64, uint4(384u, min(g_Stats[22], g_Caps1.x), 0u, 0u));
    g_Args.Store4(80, uint4(384u, min(g_Stats[28], g_Caps1.y), 0u, 0u));
}
