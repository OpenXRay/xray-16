// Simple pass-through vertex shader for RenderContext testing

struct VSInput {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;  // UV coordinates
};

struct VSOutput {
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}
