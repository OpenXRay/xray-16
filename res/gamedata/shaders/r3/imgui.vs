// ImGui Vertex Shader
// Simple vertex shader for ImGui rendering with NVRHI

cbuffer ImGuiConstants : register(b0)
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.0f, 1.0f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}