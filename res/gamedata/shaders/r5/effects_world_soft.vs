// effects_world_soft.vs — world-space FX with soft depth occlusion (rain / thunderbolt)
cbuffer SoftFXConstants : register(b0)
{
    float4x4 m_WVP;
    float4 EyePos; // xyz = camera
};

struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(m_WVP, float4(input.position, 1.0));
    output.color = input.color.bgra;
    output.texcoord = input.texcoord;
    output.worldPos = input.position;
    return output;
}
