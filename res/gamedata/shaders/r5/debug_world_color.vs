cbuffer DebugWorldCB : register(b0)
{
    float4x4 viewProj;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color    : COLOR;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color    : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = mul(viewProj, float4(input.position, 1.0));
    output.color = input.color;
    return output;
}
