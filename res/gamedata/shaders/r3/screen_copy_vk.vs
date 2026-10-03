// Diagnostic fullscreen triangle. SV_VertexID avoids a vertex buffer.
struct ScreenVertex
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

ScreenVertex main(uint vertex_id : SV_VertexID)
{
    ScreenVertex output;
    output.uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}
