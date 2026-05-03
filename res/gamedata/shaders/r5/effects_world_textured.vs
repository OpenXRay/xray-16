cbuffer DynamicTransforms : register(b0)
{
    float4x4 m_WVP;
    float4x4 m_WV;
    float4x4 m_W;
    float4 L_material;
    float4 hemi_cube_pos_faces;
    float4 hemi_cube_neg_faces;
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
    output.position = mul(m_WVP, float4(input.position, 1.0));
    output.color = input.color.bgra;
    output.texcoord = input.texcoord;
    return output;
}
