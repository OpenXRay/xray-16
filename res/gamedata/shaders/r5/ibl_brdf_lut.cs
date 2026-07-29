#define SM_6_0

RWTexture2D<float2> u_BRDFLUT : register(u0);

cbuffer BRDFLUTParams : register(b5)
{
	uint2 cb_Dims;
	uint2 cb_Pad;
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
	float a = roughness * roughness;
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(max(1.0 - cosTheta * cosTheta, 0.0));
	float3 H = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
	float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 T = normalize(cross(up, N));
	float3 B = cross(N, T);
	return normalize(T * H.x + B * H.y + N * H.z);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;
	return NdotV / max(NdotV * (1.0 - k) + k, 1e-5);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
	return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
	float3 V = float3(sqrt(max(1.0 - NdotV * NdotV, 0.0)), 0.0, NdotV);
	float A = 0.0;
	float B = 0.0;
	const uint SAMPLE_COUNT = 64u;
	float3 N = float3(0, 0, 1);
	for (uint i = 0u; i < SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
		float3 L = normalize(2.0 * dot(V, H) * H - V);
		float NdotL = saturate(L.z);
		float NdotH = saturate(H.z);
		float VdotH = saturate(dot(V, H));
		if (NdotL > 0.0)
		{
			float G = GeometrySmith(NdotV, NdotL, roughness);
			float G_Vis = (G * VdotH) / max(NdotH * NdotV, 1e-5);
			float Fc = pow(1.0 - VdotH, 5.0);
			A += (1.0 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	A /= float(SAMPLE_COUNT);
	B /= float(SAMPLE_COUNT);
	return float2(A, B);
}

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (any(dtID.xy >= cb_Dims))
		return;
	float2 uv = (float2(dtID.xy) + 0.5) / float2(cb_Dims);
	float NdotV = max(uv.x, 1e-3);
	float roughness = max(uv.y, 1e-3);
	u_BRDFLUT[dtID.xy] = IntegrateBRDF(NdotV, roughness);
}
