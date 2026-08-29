#define SM_5_0

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_ArgsOpaque : register(u0);
RWByteAddressBuffer g_ArgsTerrain : register(u1);
RWByteAddressBuffer g_ArgsAT : register(u2);

[numthreads(1, 1, 1)]
void main()
{
    g_ArgsOpaque.Store4(0, uint4(384u, g_Count.Load(0), 0u, 0u));
    g_ArgsTerrain.Store4(0, uint4(384u, g_Count.Load(4), 0u, 0u));
    g_ArgsAT.Store4(0, uint4(384u, g_Count.Load(8), 0u, 0u));
}
