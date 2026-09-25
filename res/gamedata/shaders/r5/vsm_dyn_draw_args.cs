#define SM_5_0

cbuffer VsmDynArgsParams
{
    uint g_CapOpaque;
    uint g_CapAT;
    uint g_CapSkinned;
    uint g_DynArgsPad;
};

StructuredBuffer<uint> g_Stats;
StructuredBuffer<uint> g_DynAllocInfo;
RWByteAddressBuffer g_ArgsOpaque;
RWByteAddressBuffer g_ArgsAT;
RWByteAddressBuffer g_ArgsSkinned;
RWByteAddressBuffer g_ArgsClear;

[numthreads(1, 1, 1)]
void main()
{
    g_ArgsOpaque.Store4(0, uint4(384u, min(g_Stats[4], g_CapOpaque), 0u, 0u));
    g_ArgsAT.Store4(0, uint4(384u, min(g_Stats[5], g_CapAT), 0u, 0u));
    g_ArgsSkinned.Store4(0, uint4(384u, min(g_Stats[12], g_CapSkinned), 0u, 0u));
    g_ArgsClear.Store4(0, uint4(6u, min(g_DynAllocInfo[0], 2048u), 0u, 0u));
}
