#define SM_5_0

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_Args : register(u0);
RWByteAddressBuffer g_TerrainArgs : register(u1);

[numthreads(1, 1, 1)]
void main()
{
    g_Args.Store4(0, uint4(384u, g_Count.Load(0), 0u, 0u));
    g_TerrainArgs.Store4(0, uint4(384u, g_Count.Load(4), 0u, 0u));
}
