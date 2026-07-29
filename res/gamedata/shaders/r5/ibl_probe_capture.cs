#define SM_6_0

TextureCube t_Sky0 : register(t0);
TextureCube t_Sky1 : register(t1);
Texture2D g_SceneColor : register(t2);
Texture2D g_SceneDepth : register(t3);
SamplerState smp_linear : register(s0);
SamplerState smp_nofilter : register(s1);
RWTexture2D<float4> u_ProbeFaces : register(u0);

cbuffer ProbeCaptureParams : register(b5)
{
	float4x4 cb_VP;
	float4x4 cb_InvVP;
	float3   cb_ProbePos;
	float    cb_SkyLerp;
	float3   cb_EyePos;
	float    cb_MaxDistance;
	uint2    cb_Size;
	uint     cb_Face;
	uint     cb_Pad;
};

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

bool ProjectWorld(float3 worldPos, out float2 uv, out float viewZ)
{
	float4 clip = mul(cb_VP, float4(worldPos, 1.0));
	if (clip.w <= 1e-4)
	{
		uv = 0;
		viewZ = 0;
		return false;
	}
	float3 ndc = clip.xyz / clip.w;
	uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	viewZ = abs(clip.w);
	return all(uv >= 0.0) && all(uv <= 1.0);
}

[numthreads(8, 8, 1)]
void main(uint3 dtID : SV_DispatchThreadID)
{
	if (dtID.x >= cb_Size.x || dtID.y >= cb_Size.y)
		return;

	float2 uv = (float2(dtID.xy) + 0.5) / float2(cb_Size);
	float3 dir = FaceDirection(cb_Face, uv);
	float3 sky = lerp(
		t_Sky0.SampleLevel(smp_linear, dir, 0.0).rgb,
		t_Sky1.SampleLevel(smp_linear, dir, 0.0).rgb,
		saturate(cb_SkyLerp));

	float3 color = sky;
	const uint STEPS = 16u;
	float stepLen = cb_MaxDistance / float(STEPS);
	for (uint i = 1u; i <= STEPS; ++i)
	{
		float3 p = cb_ProbePos + dir * (stepLen * float(i));
		float2 suv;
		float vz;
		if (!ProjectWorld(p, suv, vz))
			continue;
		float rawDepth = g_SceneDepth.SampleLevel(smp_nofilter, suv, 0).x;
		if (rawDepth <= 1e-7)
			continue;
		float4 worldH = mul(cb_InvVP, float4(suv.x * 2.0 - 1.0, 1.0 - suv.y * 2.0, rawDepth, 1.0));
		float3 hitPos = worldH.xyz / max(worldH.w, 1e-5);
		float dist = length(hitPos - cb_ProbePos);
		if (dist > cb_MaxDistance)
			continue;
		float align = saturate(dot(normalize(hitPos - cb_ProbePos), dir));
		if (align < 0.85)
			continue;
		float3 scene = g_SceneColor.SampleLevel(smp_linear, suv, 0).rgb;
		float w = saturate(1.0 - dist / cb_MaxDistance) * align;
		color = lerp(color, scene, w);
		break;
	}

	u_ProbeFaces[dtID.xy] = float4(color, 1.0);
}
