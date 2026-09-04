#define SM_5_0

cbuffer LocalShadowArgsParams : register(b5)
{
    uint4 g_Caps0;
    uint4 g_Caps1;
    uint g_RefreshStaticCount;
    uint g_RefreshDynCount;
    uint2 g_ArgsPad;
};

StructuredBuffer<uint> g_Stats : register(t14);
RWByteAddressBuffer g_Args : register(u0);
RWByteAddressBuffer g_ClearArgs : register(u1);

[numthreads(1, 1, 1)]
void main()
{
    g_Args.Store4(0, uint4(384u, min(g_Stats[4], g_Caps0.x), 0u, 0u));
    g_Args.Store4(16, uint4(384u, min(g_Stats[5], g_Caps0.y), 0u, 0u));
    g_Args.Store4(32, uint4(384u, min(g_Stats[6], g_Caps0.z), 0u, 0u));
    g_Args.Store4(48, uint4(384u, min(g_Stats[12], g_Caps0.w), 0u, 0u));
    g_Args.Store4(64, uint4(384u, min(g_Stats[14], g_Caps1.x), 0u, 0u));
    g_Args.Store4(80, uint4(384u, min(g_Stats[20], g_Caps1.y), 0u, 0u));
    g_ClearArgs.Store4(0, uint4(6u, g_RefreshStaticCount, 0u, 0u));
    g_ClearArgs.Store4(16, uint4(6u, g_RefreshDynCount, 0u, 0u));
}
