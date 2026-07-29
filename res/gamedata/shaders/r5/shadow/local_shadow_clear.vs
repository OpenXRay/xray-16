struct VS_OUTPUT
{
    float4 position : SV_Position;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;
    float2 uv = float2((vertexID == 1) ? 2.0 : 0.0, (vertexID == 2) ? 2.0 : 0.0);
    output.position = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 1.0, 1.0);
    return output;
}
