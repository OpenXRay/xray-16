// xrRender/Shaders/tonemap.vs.hlsl
// Fullscreen triangle vertex shader (no vertex buffer needed)

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID) {
    VS_OUTPUT output;

    // Generate fullscreen triangle using vertex ID
    // Vertex 0: (-1, -1) -> UV (0, 1)
    // Vertex 1: (-1,  3) -> UV (0, -1)
    // Vertex 2: ( 3, -1) -> UV (2, 1)
    float x = (vertexID == 2) ? 3.0 : -1.0;
    float y = (vertexID == 1) ? 3.0 : -1.0;

    output.position = float4(x, y, 0.0, 1.0);

    // Convert NDC [-1,1] to UV [0,1]
    output.texcoord = float2((x + 1.0) * 0.5, (1.0 - y) * 0.5);

    return output;
}
