// r5/shared/ssr.h — Screen-space reflections (X-Ray +Z view depth)
// Hard frustum miss made ALL wet pixels snap together (parallel R) — sky or geo.
// On border exit: clamp UV + fade conf, never binary zero.
#ifndef SSR_H_R5
#define SSR_H_R5

#define SSR_EDGE_ATTENUATION 0.10
#define SSR_VERTICAL_FADE 4.0

#if !defined(SSR_QUALITY)
	#define SSR_QUALITY 1
#endif
#if (SSR_QUALITY <= 1) || (SSR_QUALITY > 4)
	#define SSR_SAMPLES 16
	#define SSR_DISTANCE 80.0
	#define SSR_THICKNESS 1.5
	#define SSR_REFINE 2
#elif SSR_QUALITY == 2
	#define SSR_SAMPLES 24
	#define SSR_DISTANCE 120.0
	#define SSR_THICKNESS 1.25
	#define SSR_REFINE 4
#elif SSR_QUALITY == 3
	#define SSR_SAMPLES 32
	#define SSR_DISTANCE 160.0
	#define SSR_THICKNESS 1.0
	#define SSR_REFINE 6
#else
	#define SSR_SAMPLES 48
	#define SSR_DISTANCE 220.0
	#define SSR_THICKNESS 0.85
	#define SSR_REFINE 8
#endif

Texture2D g_SceneColor : register(t20);
Texture2D g_SceneDepth : register(t21);

float RayAttenBorder(float2 pos, float value)
{
	float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
	return saturate(borderDist > value ? 1.0 : borderDist / value);
}

bool WorldToUv(float3 worldPos, out float2 uv, out float w)
{
	uv = 0.0;
	float4 clip = mul(m_VP, float4(worldPos, 1.0));
	w = clip.w;
	if (w <= 1e-4)
		return false;
	float2 ndc = clip.xy / w;
	uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
	return true;
}

float3 ReconstructWorld(float2 uv, float rawDepth)
{
	float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
	float4 worldH = mul(m_InvVP, float4(ndc, rawDepth, 1.0));
	return worldH.xyz / max(worldH.w, 1e-5);
}

float SampleSceneViewZ(float2 uv, out float rawDepth)
{
	rawDepth = g_SceneDepth.SampleLevel(smp_nofilter, uv, 0).x;
	float3 worldPos = ReconstructWorld(uv, rawDepth);
	return abs(mul(m_V, float4(worldPos, 1.0)).z);
}

float4 compute_ssr_ex(
	float3 position,
	float3 normal,
	float3 skybox,
	int steps,
	float maxDist,
	float thickness,
	int refineCount,
	float roughnessBlur)
{
	float3 N = normalize(normal);
	float3 V = normalize(position - eye_position);
	float3 R = reflect(V, N);
#ifdef SSR_WATER_DOWNWARD_BIAS
	if (R.y < -0.2)
		return float4(skybox, 0.0);
#endif

#ifdef SSR_OPAQUE_REFLECTOR
	if (N.y > 0.5)
	{
		N = normalize(lerp(N, float3(0.0, 1.0, 0.0), 0.35));
		R = reflect(V, N);
		R.y = max(R.y, 0.02);
	}
#endif

	R = normalize(R);
	steps = clamp(steps, 8, 64);
	refineCount = clamp(refineCount, 1, 8);
	maxDist = max(maxDist, 8.0);
	thickness = max(thickness, 0.35);

	float3 origin = position;
	float2 startUV = 0.0;
	float startW = 1.0;
#ifdef SSR_OPAQUE_REFLECTOR
	origin = position + N * 0.45;
	thickness = max(thickness, 2.0);
	steps = max(steps, 40);
	maxDist = max(maxDist, 180.0);
#endif
	WorldToUv(origin, startUV, startW);

	float tMin = max(0.5, maxDist / float(steps) * 0.2);
#ifdef SSR_OPAQUE_REFLECTOR
	tMin = max(tMin, 1.25);
#endif

	float2 hitUV = 0.0;
	bool hit = false;
	float hitConf = 0.0;
	bool borderFade = false;

	float tPrev = tMin;
	float2 prevUV = startUV;
	{
		float3 p0 = origin + R * tMin;
		float pw;
		WorldToUv(p0, prevUV, pw);
	}

	[loop]
	for (int i = 1; i <= steps; ++i)
	{
		float u = float(i) / float(steps);
		float t = max(tMin, maxDist * u * u);
		float3 marchPos = origin + R * t;

		float2 uv;
		float w;
		if (!WorldToUv(marchPos, uv, w))
		{
			// Behind camera: keep last on-screen sample with soft conf (no hard cut)
			if (tPrev > tMin && all(prevUV >= 0.0) && all(prevUV <= 1.0))
			{
				hitUV = prevUV;
				hitConf = 0.35;
				hit = true;
				borderFade = true;
			}
			break;
		}
		if (any(uv < 0.0) || any(uv > 1.0))
		{
			// Left frustum: clamp to edge — all wet pixels share ~same R, so a hard
			// miss here snaps the WHOLE screen (sky or geo). Keep edge sample.
			if (tPrev > tMin)
				hitUV = saturate(prevUV);
			else
				hitUV = saturate(uv);
			hitConf = 0.3 * RayAttenBorder(hitUV, 0.08);
			hit = true;
			borderFade = true;
			break;
		}

		float rawDepth;
		float sceneVZ = SampleSceneViewZ(uv, rawDepth);
		float marchVZ = abs(mul(m_V, float4(marchPos, 1.0)).z);

		if (rawDepth >= 0.9995)
		{
#ifdef SSR_ACCEPT_SKY
			if (t > tMin * 1.25)
			{
				hitUV = uv;
				// Low conf — sky sample stays dim via fill lerp (avoid plate washout)
				hitConf = 0.22;
				hit = true;
				break;
			}
#endif
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float delta = sceneVZ - marchVZ;
		if (delta <= thickness && delta >= -thickness * 5.0)
		{
#ifdef SSR_OPAQUE_REFLECTOR
			if (length(uv - startUV) < 0.028)
			{
				prevUV = uv;
				tPrev = t;
				continue;
			}
#endif
			float2 uvA = prevUV;
			float2 uvB = uv;
			float distA = tPrev;
			float distB = t;
			[loop]
			for (int r = 0; r < refineCount; ++r)
			{
				float midT = 0.5 * (distA + distB);
				float2 uvM = 0.5 * (uvA + uvB);
				float dM;
				float sVZ = SampleSceneViewZ(uvM, dM);
				float mVZ = abs(mul(m_V, float4(origin + R * midT, 1.0)).z);
				if ((sVZ - mVZ) <= thickness)
				{
					uvB = uvM;
					distB = midT;
				}
				else
				{
					uvA = uvM;
					distA = midT;
				}
			}
			hitUV = 0.5 * (uvA + uvB);
#ifdef SSR_OPAQUE_REFLECTOR
			if (length(hitUV - startUV) < 0.028)
			{
				prevUV = uv;
				tPrev = t;
				continue;
			}
#endif
			float dHit;
			float sHit = SampleSceneViewZ(hitUV, dHit);
			float mHit = abs(mul(m_V, float4(origin + R * (0.5 * (distA + distB)), 1.0)).z);
			float err = abs(sHit - mHit);
			hitConf = saturate(1.0 - err / max(thickness * 2.5, 1e-3));
			hit = hitConf > 0.04;
			break;
		}
		else if (delta < -thickness * 5.0)
		{
			// Occluded: soft keep last UV instead of hard kill (reduces yaw snap)
			if (tPrev > tMin * 1.5 && all(prevUV >= 0.0) && all(prevUV <= 1.0))
			{
				hitUV = prevUV;
				hitConf = 0.25;
				hit = true;
				borderFade = true;
			}
			break;
		}

		prevUV = uv;
		tPrev = t;
	}

	if (!hit)
		return float4(skybox, 0.0);

	// Always soft-fade near screen border (even if SSR_NO_EDGE_ATTEN for water)
	float edge = RayAttenBorder(hitUV, borderFade ? 0.14 : SSR_EDGE_ATTENUATION);
#ifdef SSR_NO_EDGE_ATTEN
	if (!borderFade)
		edge = max(edge, 0.85);
#endif
#ifdef SSR_NO_VERTICAL_FADE
	float vertFade = 1.0;
#else
	float vertFade = saturate(hitUV.y * SSR_VERTICAL_FADE);
#endif
	float conf = edge * vertFade * hitConf;

	float2 texel = screen_res.zw;
	float br = saturate(roughnessBlur) * 6.0;
	float3 img = g_SceneColor.SampleLevel(smp_rtlinear, hitUV, 0).xyz;
	if (br > 0.01)
	{
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2( br, 0) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(-br, 0) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(0,  br) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(0, -br) * texel, 0).xyz;
		img *= 0.2;
	}
	float hitLum = dot(max(img, 0.0), float3(0.2126, 0.7152, 0.0722));
#ifdef SSR_KEEP_DIM_HITS
	float keepDim = step(1e-5, hitLum);
	conf *= keepDim;
	img *= keepDim;
#else
	if (hitLum < 1e-4)
		return float4(skybox, 0.0);
	conf *= saturate(hitLum * 6.0 + 0.2);
#endif
	// On miss-like border: blend toward skybox so content doesn't pop to black
	if (borderFade)
		img = lerp(skybox, img, saturate(conf * 1.5));
	return float4(img, conf);
}

float4 compute_ssr(float3 position, float3 normal, float3 skybox)
{
	return compute_ssr_ex(
		position, normal, skybox,
		SSR_SAMPLES, SSR_DISTANCE, SSR_THICKNESS, SSR_REFINE, 0.0);
}

#endif
