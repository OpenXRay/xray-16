struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID) {
    VS_OUTPUT output;

    float x = (vertexID == 2) ? 3.0 : -1.0;
    float y = (vertexID == 1) ? 3.0 : -1.0;

    output.position = float4(x, y, 0.0, 1.0);

    output.texcoord = float2((x + 1.0) * 0.5, (1.0 - y) * 0.5);

    return output;
}
