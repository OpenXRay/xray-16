Texture2D<float> t_Depth : register(t0);
RWTexture2D<float> u_DepthR32 : register(u0);

cbuffer CopyDepthParams : register(b0) {
    uint g_Width;
    uint g_Height;
    uint g_Pad0;
    uint g_Pad1;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_Width || id.y >= g_Height)
        return;
    u_DepthR32[id.xy] = t_Depth.Load(int3(id.xy, 0));
}
