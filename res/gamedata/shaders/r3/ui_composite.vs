// UI Composite Pass - Fullscreen Triangle Vertex Shader
// Generates a fullscreen triangle using SV_VertexID (no vertex buffer needed)

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
    VS_OUTPUT output;

    // Generate fullscreen triangle
    // Vertex 0: (-1, -1) -> texcoord (0, 1)
    // Vertex 1: (-1,  3) -> texcoord (0, -1)
    // Vertex 2: ( 3, -1) -> texcoord (2, 1)
    // This covers the entire screen with one triangle

    float2 texcoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
    output.texcoord = texcoord;

    return output;
}
