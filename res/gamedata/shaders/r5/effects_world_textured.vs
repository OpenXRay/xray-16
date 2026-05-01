cbuffer EffectsWorldCB : register(b0)
{
    float4x4 m_VP;
    float4x4 m_W;
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
};

VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(m_W, float4(input.position, 1.0));
    output.position = mul(m_VP, worldPos);
    output.color = input.color.bgra;
    output.texcoord = input.texcoord;
    return output;
}
