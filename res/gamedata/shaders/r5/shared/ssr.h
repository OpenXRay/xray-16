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

// Eye-space distance to reconstructed scene position (used by SSGI march).
float SampleSceneDistance(float2 uv, out float3 scenePos, out float rawDepth)
{
	rawDepth = g_SceneDepth.SampleLevel(smp_nofilter, uv, 0).x;
	scenePos = ReconstructWorld(uv, rawDepth);
	return length(scenePos - eye_position);
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
	if (R.y < -0.55)
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

	float viewZ0 = abs(mul(m_V, float4(position, 1.0)).z);
	float nearAmt = saturate(1.0 - (viewZ0 - 0.25) / 3.0);

	float3 origin = position;
	float2 startUV = 0.0;
	float startW = 1.0;
#ifdef SSR_OPAQUE_REFLECTOR
	float bias = lerp(0.45, 0.03, nearAmt);
	origin = position + N * bias;
	thickness = max(thickness, lerp(2.0, 0.35, nearAmt));
	steps = max(steps, nearAmt > 0.4 ? 32 : 40);
	maxDist = max(maxDist, lerp(180.0, 28.0, nearAmt));
#endif
	WorldToUv(origin, startUV, startW);

	float tMin = max(0.5, maxDist / float(steps) * 0.2);
#ifdef SSR_OPAQUE_REFLECTOR
	tMin = lerp(max(tMin, 1.25), 0.04, nearAmt);
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

		if (IsSkyDepth(rawDepth))
		{
#ifdef SSR_ACCEPT_SKY
			if (t > tMin * 1.25)
			{
				hitUV = uv;
				hitConf = 0.06;
				hit = true;
				break;
			}
#endif
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float delta = sceneVZ - marchVZ;
		if (delta <= thickness && delta >= -thickness)
		{
#ifdef SSR_OPAQUE_REFLECTOR
			float minUvSep = lerp(0.028, 0.0035, nearAmt);
			if (length(uv - startUV) < minUvSep)
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
			if (length(hitUV - startUV) < minUvSep)
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
			float3 hitPos = ReconstructWorld(hitUV, dHit);
			float toward = dot(normalize(hitPos - origin), R);
			hitConf *= saturate(toward * 2.0);
			hit = hitConf > 0.04;
			break;
		}
		else if (delta < -thickness)
		{
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
	float br = saturate(roughnessBlur) * 3.5;
	float3 img = g_SceneColor.SampleLevel(smp_nofilter, hitUV, 0).xyz;
	if (br > 0.08)
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
	if (borderFade)
		img = lerp(skybox * 0.2, img, saturate(conf * 2.4));
	return float4(img, conf);
}

float4 compute_ssr(float3 position, float3 normal, float3 skybox)
{
	return compute_ssr_ex(
		position, normal, skybox,
		SSR_SAMPLES, SSR_DISTANCE, SSR_THICKNESS, SSR_REFINE, 0.0);
}

float4 compute_ssr_water(float3 position, float3 normal, float3 skybox)
{
	float3 N = normalize(lerp(normalize(normal), float3(0.0, 1.0, 0.0), 0.7));
	float3 V = normalize(position - eye_position);
	float3 R = reflect(V, N);
	if (R.y < 0.02)
		R = normalize(float3(R.x, 0.02, R.z));
	R = normalize(R);

	const int steps = 36;
	const float maxDist = 160.0;
	const float thickness = 1.15;
	const int refineCount = 6;
	const float minUvSep = 0.008;
	const float tMin = 0.85;

	float3 origin = position + N * 0.1;
	float2 startUV = 0.0;
	float startW = 1.0;
	if (!WorldToUv(origin, startUV, startW))
		return float4(skybox, 0.0);

	float2 dirUV = 0.0;
	{
		float2 uv1 = 0.0;
		float w1 = 1.0;
		if (!WorldToUv(origin + R * 3.0, uv1, w1))
			return float4(skybox, 0.0);
		dirUV = uv1 - startUV;
		float dirLen = length(dirUV);
		if (dirLen < 1e-4)
			return float4(skybox, 0.0);
		dirUV /= dirLen;
	}

	float2 hitUV = 0.0;
	bool hit = false;
	float hitConf = 0.0;
	float hitT = 0.0;

	float tPrev = tMin;
	float2 prevUV = startUV;
	{
		float pw = 1.0;
		WorldToUv(origin + R * tMin, prevUV, pw);
	}

	[loop]
	for (int i = 1; i <= steps; ++i)
	{
		float u = float(i) / float(steps);
		float t = max(tMin, maxDist * u * u);
		float3 marchPos = origin + R * t;

		float2 uv = 0.0;
		float w = 1.0;
		if (!WorldToUv(marchPos, uv, w))
			break;
		if (any(uv < 0.0) || any(uv > 1.0))
			break;

		float rawDepth = 0.0;
		float sceneVZ = SampleSceneViewZ(uv, rawDepth);
		if (IsSkyDepth(rawDepth))
		{
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float3 scenePos = ReconstructWorld(uv, rawDepth);
		if (scenePos.y < position.y - 0.2)
		{
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float3 toScene = scenePos - origin;
		float along = dot(toScene, R);
		if (along < tMin)
		{
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float perp = length(scenePos - (origin + R * along));
		float marchVZ = abs(mul(m_V, float4(marchPos, 1.0)).z);
		float delta = sceneVZ - marchVZ;
		float thick = thickness * (1.0 + along * 0.012);
		if (delta > thick || delta < -thick * 2.0 || perp > thick)
		{
			if (delta < -thick * 2.0)
				break;
			prevUV = uv;
			tPrev = t;
			continue;
		}

		if (length(uv - startUV) < minUvSep)
		{
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float2 travel = uv - startUV;
		float travelLen = length(travel);
		if (travelLen < minUvSep || dot(travel / travelLen, dirUV) < 0.2)
		{
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float distA = tPrev;
		float distB = t;
		[loop]
		for (int r = 0; r < refineCount; ++r)
		{
			float midT = 0.5 * (distA + distB);
			float2 uvM = 0.0;
			float wM = 1.0;
			WorldToUv(origin + R * midT, uvM, wM);
			float dM = 0.0;
			float sVZ = SampleSceneViewZ(uvM, dM);
			float mVZ = abs(mul(m_V, float4(origin + R * midT, 1.0)).z);
			if ((sVZ - mVZ) <= thickness)
				distB = midT;
			else
				distA = midT;
		}

		hitT = 0.5 * (distA + distB);
		float wH = 1.0;
		WorldToUv(origin + R * hitT, hitUV, wH);
		if (any(hitUV < 0.0) || any(hitUV > 1.0) || length(hitUV - startUV) < minUvSep)
			break;

		float dHit = 0.0;
		float sHit = SampleSceneViewZ(hitUV, dHit);
		if (IsSkyDepth(dHit))
			break;
		float3 hitPos = ReconstructWorld(hitUV, dHit);
		if (hitPos.y < position.y - 0.2)
			break;

		float alongH = dot(hitPos - origin, R);
		float perpH = length(hitPos - (origin + R * alongH));
		float mHit = abs(mul(m_V, float4(origin + R * hitT, 1.0)).z);
		float err = abs(sHit - mHit);
		hitConf = saturate(1.0 - err / max(thickness * 2.0, 1e-3));
		hitConf *= saturate(1.0 - perpH / max(thickness * 2.5, 1e-3));
		hitConf *= RayAttenBorder(hitUV, SSR_EDGE_ATTENUATION);
		hitConf *= saturate((alongH - tMin) * 0.5);
		hit = hitConf > 0.08;
		break;
	}

	if (!hit)
		return float4(skybox, 0.0);

	float3 img = g_SceneColor.SampleLevel(smp_nofilter, hitUV, 0).xyz;
	float hitLum = dot(max(img, 0.0), float3(0.2126, 0.7152, 0.0722));
	if (hitLum < 1e-5)
		return float4(skybox, 0.0);
	hitConf *= saturate(hitLum * 2.5 + 0.45);
	return float4(img, saturate(hitConf * 1.25));
}

float4 compute_ssr_near(
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
	float3 R = normalize(reflect(V, N));
	steps = clamp(steps, 12, 48);
	refineCount = clamp(refineCount, 1, 6);
	maxDist = clamp(maxDist, 4.0, 48.0);
	thickness = max(thickness, 0.2);

	float3 origin = position + N * 0.02;
	float2 startUV = 0.0;
	float startW = 1.0;
	WorldToUv(origin, startUV, startW);

	float tMin = 0.04;
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
			if (tPrev > tMin && all(prevUV >= 0.0) && all(prevUV <= 1.0))
			{
				hitUV = prevUV;
				hitConf = 0.4;
				hit = true;
				borderFade = true;
			}
			break;
		}
		if (any(uv < 0.0) || any(uv > 1.0))
		{
			hitUV = saturate(tPrev > tMin ? prevUV : uv);
			hitConf = 0.35 * RayAttenBorder(hitUV, 0.08);
			hit = true;
			borderFade = true;
			break;
		}

		float rawDepth;
		float sceneVZ = SampleSceneViewZ(uv, rawDepth);
		float marchVZ = abs(mul(m_V, float4(marchPos, 1.0)).z);

		if (IsSkyDepth(rawDepth))
		{
			if (t > tMin * 1.5)
			{
				hitUV = uv;
				hitConf = 0.05;
				hit = true;
				break;
			}
			prevUV = uv;
			tPrev = t;
			continue;
		}

		float delta = sceneVZ - marchVZ;
		if (delta <= thickness && delta >= -thickness)
		{
			if (length(uv - startUV) < 0.0025)
			{
				prevUV = uv;
				tPrev = t;
				continue;
			}
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
			float dHit;
			float sHit = SampleSceneViewZ(hitUV, dHit);
			float mHit = abs(mul(m_V, float4(origin + R * (0.5 * (distA + distB)), 1.0)).z);
			float err = abs(sHit - mHit);
			hitConf = saturate(1.0 - err / max(thickness * 2.0, 1e-3));
			float3 hitPos = ReconstructWorld(hitUV, dHit);
			float toward = dot(normalize(hitPos - origin), R);
			hitConf *= saturate(toward * 2.0);
			hit = hitConf > 0.03;
			break;
		}
		else if (delta < -thickness)
		{
			if (tPrev > tMin * 1.2 && all(prevUV >= 0.0) && all(prevUV <= 1.0))
			{
				hitUV = prevUV;
				hitConf = 0.3;
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

	float edge = RayAttenBorder(hitUV, borderFade ? 0.1 : 0.04);
	float conf = max(edge, 0.65) * hitConf;
	float2 texel = screen_res.zw;
	float br = saturate(roughnessBlur) * 2.0;
	float3 img = g_SceneColor.SampleLevel(smp_nofilter, hitUV, 0).xyz;
	if (br > 0.08)
	{
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2( br, 0) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(-br, 0) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(0,  br) * texel, 0).xyz;
		img += g_SceneColor.SampleLevel(smp_rtlinear, hitUV + float2(0, -br) * texel, 0).xyz;
		img *= 0.2;
	}
	float hitLum = dot(max(img, 0.0), float3(0.2126, 0.7152, 0.0722));
	conf *= step(1e-5, hitLum);
	img *= step(1e-5, hitLum);
	if (borderFade)
		img = lerp(skybox * 0.18, img, saturate(conf * 2.6));
	conf = saturate(conf);
	return float4(img, conf);
}

#endif
