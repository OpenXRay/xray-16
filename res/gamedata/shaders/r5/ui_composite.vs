struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    float2 texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
    output.texcoord = texcoord;

    return output;
}
