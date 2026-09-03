#define SM_5_0

ByteAddressBuffer g_Count : register(t0);
RWByteAddressBuffer g_ArgsSkinned : register(u0);

[numthreads(1, 1, 1)]
void main()
{
    g_ArgsSkinned.Store4(0, uint4(384u, g_Count.Load(0), 0u, 0u));
}
