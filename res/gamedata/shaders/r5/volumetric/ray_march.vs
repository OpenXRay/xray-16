// ray_march.vs — fullscreen triangle
struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float2 texcoord : TEXCOORD0;
};

VS_OUTPUT main(uint vertexID : SV_VertexID)
{
	VS_OUTPUT o;
	o.texcoord = float2((vertexID == 1) ? 2.0 : 0.0, (vertexID == 2) ? 2.0 : 0.0);
	o.position = float4(o.texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	return o;
}
