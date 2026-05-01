cbuffer DebugScreenCB : register(b0)
{
    float2 invHalfScreen;
    float2 _pad;
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
    float2 ndc;
    ndc.x = input.position.x * invHalfScreen.x - 1.0;
    ndc.y = 1.0 - input.position.y * invHalfScreen.y;
    output.position = float4(ndc, 0.0, 1.0);
    output.color = input.color;
    return output;
}
