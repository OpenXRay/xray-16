cbuffer FontCB : register(b0)
{
    float4 screen_res; // x=width, y=height, z=1/width, w=1/height
};

struct VSInput
{
    float4 position : POSITION;
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
    float2 px = input.position.xy + 0.5f;
    output.position.x = px.x * screen_res.z * 2.0f - 1.0f;
    output.position.y = -(px.y * screen_res.w * 2.0f - 1.0f);
    output.position.zw = input.position.zw;
    output.color = input.color.bgra;
    output.texcoord = input.texcoord;
    return output;
}
