// xrRender/Shaders/fullscreen.vs.hlsl
// Fullscreen triangle vertex shader

struct VS_OUTPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

// Fullscreen triangle trick
// Vertices: (-1,-1), (3,-1), (-1,3)
// Covers entire screen with one triangle
VS_OUTPUT main(uint vertexID : SV_VertexID) {
    VS_OUTPUT output;

    // Generate fullscreen triangle positions
    output.texcoord = float2(
        (vertexID == 1) ? 2.0 : 0.0,
        (vertexID == 2) ? 2.0 : 0.0
    );

    output.position = float4(
        output.texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0),
        0.0,
        1.0
    );

    return output;
}
