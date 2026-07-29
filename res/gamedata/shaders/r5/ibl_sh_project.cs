#define SM_6_0

TextureCube t_Source : register(t0);
SamplerState smp_linear : register(s0);
RWStructuredBuffer<float4> u_SH : register(u0);

cbuffer SHProjectParams : register(b5)
{
	uint cb_Size;
	uint cb_OutOffset;
	uint2 cb_Pad;
};

static const float PI = 3.14159265359;

float3 FaceDirection(uint face, float2 uv)
{
	float2 st = uv * 2.0 - 1.0;
	if (face == 0u) return normalize(float3( 1.0, -st.y, -st.x));
	if (face == 1u) return normalize(float3(-1.0, -st.y,  st.x));
	if (face == 2u) return normalize(float3( st.x,  1.0,  st.y));
	if (face == 3u) return normalize(float3( st.x, -1.0, -st.y));
	if (face == 4u) return normalize(float3( st.x, -st.y,  1.0));
	return normalize(float3(-st.x, -st.y, -1.0));
}

void SHAdd(inout float3 sh[9], float3 dir, float3 color, float weight)
{
	float x = dir.x;
	float y = dir.y;
	float z = dir.z;
	float3 c = color * weight;
	sh[0] += c * 0.282095;
	sh[1] += c * (-0.488603 * y);
	sh[2] += c * (0.488603 * z);
	sh[3] += c * (-0.488603 * x);
	sh[4] += c * (1.092548 * x * y);
	sh[5] += c * (-1.092548 * y * z);
	sh[6] += c * (0.315392 * (3.0 * z * z - 1.0));
	sh[7] += c * (-1.092548 * x * z);
	sh[8] += c * (0.546274 * (x * x - y * y));
}

[numthreads(1, 1, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	float3 sh[9];
	[unroll] for (uint k = 0; k < 9; ++k)
		sh[k] = 0.0.xxx;

	uint size = max(cb_Size, 1u);
	for (uint face = 0u; face < 6u; ++face)
	{
		for (uint y = 0u; y < size; ++y)
		{
			for (uint x = 0u; x < size; ++x)
			{
				float2 uv = (float2(x, y) + 0.5) / float(size);
				float3 dir = FaceDirection(face, uv);
				float2 st = uv * 2.0 - 1.0;
				float inv = rcp(max(dot(st, st) + 1.0, 1e-4));
				float solidAngle = 4.0 * inv * sqrt(inv);
				solidAngle *= (4.0 * PI) / (6.0 * float(size * size));
				float3 color = t_Source.SampleLevel(smp_linear, dir, 0.0).rgb;
				SHAdd(sh, dir, color, solidAngle);
			}
		}
	}

	[unroll] for (uint o = 0; o < 9; ++o)
		u_SH[cb_OutOffset + o] = float4(sh[o], 0.0);
}
