#define SM_5_0

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_Args : register(u0);

[numthreads(1, 1, 1)]
void main()
{
    uint count = g_Count.Load(0);
    g_Args.Store4(0, uint4(384u, count, 0u, 0u));
}
