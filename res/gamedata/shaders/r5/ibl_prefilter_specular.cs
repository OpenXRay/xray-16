#define SM_6_0

TextureCube t_Source : register(t0);
SamplerState smp_linear : register(s0);
RWTexture2D<float4> u_Dest : register(u0);

cbuffer PrefilterParams : register(b5)
{
	uint2 cb_Size;
	uint  cb_Face;
	uint  cb_SampleCount;
	float cb_Roughness;
	float cb_MipBias;
	float2 cb_Pad;
};

static const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
	return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = max(roughness * roughness, 1e-4);
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
	float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 T = normalize(cross(up, N));
	float3 B = cross(N, T);
	return normalize(T * H.x + B * H.y + N * H.z);
}

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

float DistributionGGX(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 1e-6);
}

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (dtID.x >= cb_Size.x || dtID.y >= cb_Size.y)
		return;

	float2 uv = (float2(dtID.xy) + 0.5) / float2(cb_Size);
	float3 N = FaceDirection(cb_Face, uv);
	float3 R = N;
	float3 V = R;

	float3 prefiltered = 0.0.xxx;
	float totalWeight = 0.0;
	uint sampleCount = max(cb_SampleCount, 1u);

	if (cb_Roughness < 1e-3)
	{
		prefiltered = t_Source.SampleLevel(smp_linear, N, cb_MipBias).rgb;
		totalWeight = 1.0;
	}
	else
	{
		for (uint i = 0u; i < sampleCount; ++i)
		{
			float2 Xi = Hammersley(i, sampleCount);
			float3 H = ImportanceSampleGGX(Xi, N, cb_Roughness);
			float3 L = normalize(2.0 * dot(V, H) * H - V);
			float NdotL = saturate(dot(N, L));
			if (NdotL > 0.0)
			{
				float NdotH = saturate(dot(N, H));
				float HdotV = saturate(dot(H, V));
				float D = DistributionGGX(NdotH, cb_Roughness);
				float pdf = D * NdotH / max(4.0 * HdotV, 1e-4);
				float saTexel = 4.0 * PI / (6.0 * float(cb_Size.x * cb_Size.y));
				float saSample = 1.0 / max(float(sampleCount) * pdf, 1e-4);
				float mip = cb_MipBias + 0.5 * log2(max(saSample / saTexel, 1e-4));
				prefiltered += t_Source.SampleLevel(smp_linear, L, mip).rgb * NdotL;
				totalWeight += NdotL;
			}
		}
	}

	prefiltered /= max(totalWeight, 1e-4);
	u_Dest[dtID.xy] = float4(prefiltered, 1.0);
}
