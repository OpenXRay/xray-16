// shared/shadow_sampling.h — CSM + soft PCSS (FrameGraph r5)
#ifndef SHADOW_SAMPLING_H
#define SHADOW_SAMPLING_H

#ifdef CSM_SHADOW_FORWARD
// Per-cascade maps (resolution ladder). Texture2D — not Array: one SampleCmp
// target per cascade avoids layout/slot mismatches with a shared array.
Texture2D<float> g_ShadowMap0 : register(t23);
Texture2D<float> g_ShadowMap1 : register(t29);
Texture2D<float> g_ShadowMap2 : register(t30);
Texture2D<float> g_ShadowHZB0 : register(t31);
Texture2D<float> g_ShadowHZB1 : register(t32);
Texture2D<float> g_ShadowHZB2 : register(t33);
SamplerComparisonState smp_shadowcmp : register(s4);
SamplerState smp_shadow_point : register(s5);
#endif

#ifdef HUD_SHADOW_FORWARD
Texture2DArray<float> g_HUDShadowMap : register(t24);
#endif

#ifdef LOCAL_SHADOW_FORWARD
Texture2DArray<float> g_LocalShadowAtlas : register(t27);
Texture2DArray<float> g_LocalShadowESM : register(t51);
#ifndef CSM_SHADOW_FORWARD
SamplerComparisonState smp_shadowcmp : register(s4);
#endif
#ifndef CSM_SHADOW_FORWARD
SamplerState smp_shadow_point : register(s5);
#endif
#endif

#ifdef SHADOW_MASK_FORWARD
Texture2D g_ShadowMask : register(t34);

bool ShadowMaskEnabled()
{
	return dev_param_2.z > 0.5;
}

float3 SampleShadowMask(float2 screenUV)
{
	return g_ShadowMask.SampleLevel(smp_rtlinear, screenUV, 0).rgb;
}
#endif

#include "shared/contact_shadows.h"

#ifdef CSM_SHADOW_FORWARD

float ShadowCmpCascade(uint cascade, float2 uv, float refDepth)
{
	if (cascade == 0)
		return g_ShadowMap0.SampleCmp(smp_shadowcmp, uv, refDepth);
	if (cascade == 1)
		return g_ShadowMap1.SampleCmp(smp_shadowcmp, uv, refDepth);
	return g_ShadowMap2.SampleCmp(smp_shadowcmp, uv, refDepth);
}

// NVRHI Vulkan comparison: CompareOp::Less → 1 = lit, 0 = shadowed.
// uvw.xy = shadow UV, uvw.z = cascade index (matches previous Array API).
float ShadowCmpLit(float3 uvw, float refDepth)
{
	return ShadowCmpCascade((uint)uvw.z, uvw.xy, refDepth);
}

float ReceiverBias(uint cascade)
{
	// Small depth bias; the bulk of acne removal is done by world normal-offset
	// (see NormalOffsetWorld), so this stays low to avoid peter-panning.
	return 0.0004 + 0.0003 * float(cascade);
}

// World-space normal offset before projecting into the shadow map. Pushing the
// sample off the surface along N removes self-shadow acne without detaching the
// contact (peter-panning) the way a large depth bias does. dev_param_3.w =
// r2_sun_normal_bias (meters); coarser cascades cover more world/texel so scale up.
float3 NormalOffsetWorld(float3 N, uint cascade)
{
	float baseMeters = dev_param_3.w > 0.0 ? dev_param_3.w : 0.04;
	return N * (baseMeters * (1.0 + float(cascade)));
}

float ShadowDepth(uint cascade, float2 uv)
{
	if (cascade == 0)
		return g_ShadowMap0.SampleLevel(smp_shadow_point, uv, 0).x;
	if (cascade == 1)
		return g_ShadowMap1.SampleLevel(smp_shadow_point, uv, 0).x;
	return g_ShadowMap2.SampleLevel(smp_shadow_point, uv, 0).x;
}

float CascadeSmapSize(uint cascade)
{
	float base = dev_param_3.x > 0 ? dev_param_3.x : 2048.0;
	return max(base / float(1u << cascade), 512.0);
}

bool CascadeCoordValid(float4 sc)
{
	return all(sc.xy >= 0.0) && all(sc.xy <= 1.0) && sc.z >= 0.0 && sc.z <= 1.0;
}

float CascadeUVRim(float2 uv)
{
	float2 d = min(uv, 1.0 - uv);
	return min(d.x, d.y);
}

float ShadowHZBMax(uint cascade, float2 uv, float mip)
{
	if (cascade == 0)
		return g_ShadowHZB0.SampleLevel(smp_shadow_point, uv, mip).x;
	if (cascade == 1)
		return g_ShadowHZB1.SampleLevel(smp_shadow_point, uv, mip).x;
	return g_ShadowHZB2.SampleLevel(smp_shadow_point, uv, mip).x;
}

bool ShadowHZBEnabled()
{
	return dev_param_2.y > 0.5;
}

float TryShadowHZBEarlyOut(uint cascade, float2 shadowUV, float refDepth)
{
#if defined(SHADOW_MASK_CS)
	return -1.0;
#else
	if (!ShadowHZBEnabled())
		return -1.0;
	float smapSize = CascadeSmapSize(cascade);
	float2 hzbSize = float2(smapSize, smapSize) * 0.5;
	float2 dx = abs(ddx(shadowUV)) * hzbSize;
	float2 dy = abs(ddy(shadowUV)) * hzbSize;
	float footprint = max(max(dx.x, dx.y), max(dy.x, dy.y));
	float mip = clamp(log2(max(footprint, 1.0)), 0.0, 6.0);
	float maxD = ShadowHZBMax(cascade, shadowUV, mip);
	float rd = saturate(refDepth - ReceiverBias(cascade));
	if (rd > maxD + 1e-4)
		return 0.0;
	return -1.0;
#endif
}

float SampleCascadePCF(uint cascade, float2 shadowUV, float refDepth, float softScale)
{
	float hzb = TryShadowHZBEarlyOut(cascade, shadowUV, refDepth);
	if (hzb >= 0.0)
		return hzb;
	float smapSize = CascadeSmapSize(cascade);
	float dsm = max(dev_param_1.x, 0.1);
	float ts = (0.75 * max(softScale, 1.0) * (dsm / 0.7)) / smapSize;
	float rd = saturate(refDepth - ReceiverBias(cascade));
	float lit = 0.0;
	[unroll]
	for (int y = -1; y <= 1; ++y)
	{
		[unroll]
		for (int x = -1; x <= 1; ++x)
		{
			float2 o = float2((float)x, (float)y) * ts;
			lit += ShadowCmpLit(float3(shadowUV + o, (float)cascade), rd);
		}
	}
	return lit * (1.0 / 9.0);
}

float2 VogelDisk(uint i, uint n)
{
	float r = sqrt((float(i) + 0.5) / float(n));
	float theta = float(i) * 2.399963229728653;
	return float2(r * cos(theta), r * sin(theta));
}

float2x2 ShadowDiskRotation(float2 shadowUV, float smapSize)
{
	float n = frac(sin(dot(shadowUV * smapSize, float2(12.9898, 78.233))) * 43758.5453);
	float a = n * 6.28318530718;
	float ca = cos(a);
	float sa = sin(a);
	return float2x2(ca, -sa, sa, ca);
}

float FindAverageBlockerDepth5(
	uint cascade,
	float2 shadowUV,
	float refDepth,
	float searchRadiusUV,
	out float blockerCount)
{
	float avgBlocker = 0.0;
	blockerCount = 0.0;
	float thresh = refDepth - ReceiverBias(cascade) - 1e-5;

	[unroll]
	for (int y = -2; y <= 2; ++y)
	{
		[unroll]
		for (int x = -2; x <= 2; ++x)
		{
			float2 uv = shadowUV + float2((float)x, (float)y) * searchRadiusUV;
			float d = ShadowDepth(cascade, uv);
			if (d < thresh)
			{
				avgBlocker += d;
				blockerCount += 1.0;
			}
		}
	}

	if (blockerCount > 1e-5)
		return avgBlocker / blockerCount;
	return 0.0;
}

float FindAverageBlockerDepth7(
	uint cascade,
	float2 shadowUV,
	float refDepth,
	float searchRadiusUV,
	out float blockerCount)
{
	float avgBlocker = 0.0;
	blockerCount = 0.0;
	float thresh = refDepth - ReceiverBias(cascade) - 1e-5;

	[unroll]
	for (int y = -3; y <= 3; ++y)
	{
		[unroll]
		for (int x = -3; x <= 3; ++x)
		{
			float2 uv = shadowUV + float2((float)x, (float)y) * searchRadiusUV;
			float d = ShadowDepth(cascade, uv);
			if (d < thresh)
			{
				avgBlocker += d;
				blockerCount += 1.0;
			}
		}
	}

	if (blockerCount > 1e-5)
		return avgBlocker / blockerCount;
	return 0.0;
}

float FilterPCSSPoisson(
	uint cascade,
	float2 shadowUV,
	float refDepth,
	float filterRadiusUV,
	float smapSize,
	uint tapCount)
{
	float rd = saturate(refDepth - ReceiverBias(cascade));
	float2x2 rot = ShadowDiskRotation(shadowUV, smapSize);
	float sum = 0.0;

	[loop]
	for (uint i = 0; i < tapCount; ++i)
	{
		float2 o = mul(rot, VogelDisk(i, tapCount)) * filterRadiusUV;
		sum += ShadowCmpLit(float3(shadowUV + o, (float)cascade), rd);
	}

	return sum / float(tapCount);
}

float SampleCascadePCSS(uint cascade, float2 shadowUV, float refDepth, float viewZ, float midSplit)
{
	float hzb = TryShadowHZBEarlyOut(cascade, shadowUV, refDepth);
	if (hzb >= 0.0)
		return hzb;
	float smapSize = CascadeSmapSize(cascade);
	float sunQuality = dev_param_3.y;
	float dsm = max(dev_param_1.x, 0.1) / 0.7;

	float softTexels    = (dev_param_4.x > 0.0 ? dev_param_4.x : 6.0) * dsm;
	float blockerTexels = (dev_param_4.y > 0.0 ? dev_param_4.y : 2.5) * dsm;
	float contactTexels = (dev_param_4.z > 0.0 ? dev_param_4.z : 1.0) * dsm;

	if (sunQuality < 0.5)
		return SampleCascadePCF(cascade, shadowUV, refDepth, contactTexels / 0.6);

	float blockerSearchUV = max(blockerTexels, 0.5) / smapSize;
	float blockerCount = 0.0;
	float avgBlocker;
	if (sunQuality >= 2.5)
		avgBlocker = FindAverageBlockerDepth7(
			cascade, shadowUV, refDepth, blockerSearchUV, blockerCount);
	else
		avgBlocker = FindAverageBlockerDepth5(
			cascade, shadowUV, refDepth, blockerSearchUV, blockerCount);

	if (blockerCount <= 0.12)
		return SampleCascadePCF(cascade, shadowUV, refDepth, contactTexels / 0.6);

	float gap = max(refDepth - avgBlocker, 0.0);
	float penumbra = saturate(gap / max(avgBlocker, 1e-3) * 3.0);

	float cascadeScale = 1.0 + float(cascade) * 0.3;
	float maxTexels = softTexels * cascadeScale;
	float widthTexels = clamp(lerp(contactTexels, maxTexels, penumbra), contactTexels, maxTexels);
	float filterRadiusUV = widthTexels / smapSize;

	uint taps = 16;
	if (sunQuality >= 2.5)
		taps = 32;
	else if (sunQuality >= 1.5)
		taps = 24;

	return FilterPCSSPoisson(cascade, shadowUV, refDepth, filterRadiusUV, smapSize, taps);
}

// Try cascade c with full PCSS; returns lit in [0,1] or -1 if UV/depth invalid.
float TrySampleCascadePCSS(uint c, float3 worldPos, float3 N, float viewZ, float midSplit)
{
	float3 wp = worldPos + NormalOffsetWorld(N, c);
	float4 sc = mul(shadow_matrices[c], float4(wp, 1.0));
	if (!CascadeCoordValid(sc))
		return -1.0;
	return SampleCascadePCSS(c, sc.xy, sc.z, viewZ, midSplit);
}

// Cheap 4-tap PCF for cascade blend / rim / grass — avoids a second full PCSS.
float TrySampleCascadePCF(uint c, float3 worldPos, float3 N)
{
	float3 wp = worldPos + NormalOffsetWorld(N, c);
	float4 sc = mul(shadow_matrices[c], float4(wp, 1.0));
	if (!CascadeCoordValid(sc))
		return -1.0;
	return SampleCascadePCF(c, sc.xy, sc.z, 1.0);
}

// Depth-selected CSM with smooth split blend + UV rim handoff.
// Full PCSS only on the primary cascade; blends use cheap PCF (was 2–3× PCSS/pixel).
float SampleCSM(float3 worldPos, float3 N)
{
	const uint cascadeCount = 3;
	float3 splits = cascade_splits.xyz;
	float rimWidth = cascade_splits.w > 1e-4 ? cascade_splits.w : 0.12;
	float midSplit = splits.y;
	float farSplit = max(splits.z, midSplit + 1.0);

	float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);

	// Soft end of shadow distance. r2_slight_fade (dev_param_1.w) extends how far
	// CSM stays opaque — classic used the same cvar for light shadow LOD.
	float slightFade = saturate(max(dev_param_1.w, 0.2));
	float fadeStart = farSplit * lerp(0.78, 0.92, slightFade);
	float rangeFade = 1.0 - smoothstep(fadeStart, farSplit, viewZ);
	if (rangeFade <= 1e-3)
		return 1.0;

	uint primary = 2;
	if (viewZ < splits.x)
		primary = 0;
	else if (viewZ < splits.y)
		primary = 1;

	float blendStart = 0.0;
	float blendEnd = 0.0;
	uint nextC = primary;
	if (primary == 0)
	{
		blendEnd = splits.x;
		blendStart = splits.x * 0.65; // was 0.82 — hard pop a few metres ahead
		nextC = 1;
	}
	else if (primary == 1)
	{
		blendEnd = splits.y;
		blendStart = lerp(splits.x, splits.y, 0.65);
		nextC = 2;
	}

	float litPrimary = -1.0;
	uint used = primary;
	[unroll]
	for (uint c = primary; c < cascadeCount; ++c)
	{
		float lit = TrySampleCascadePCSS(c, worldPos, N, viewZ, midSplit);
		if (lit >= 0.0)
		{
			litPrimary = lit;
			used = c;
			break;
		}
	}

	// Outside all cascade UVs: fully lit (never clamp-sample edge texels — that
	// stretched a hard dark disc matching the far-cascade footprint).
	if (litPrimary < 0.0)
		return 1.0;

	float result = litPrimary;

	// Split-distance blend toward next coarser cascade (cheap PCF)
	if (primary < 2 && used == primary)
	{
		float splitBlend = saturate((viewZ - blendStart) / max(blendEnd - blendStart, 1e-3));
		splitBlend = smoothstep(0.0, 1.0, splitBlend);
		if (splitBlend > 1e-3)
		{
			float litNext = TrySampleCascadePCF(nextC, worldPos, N);
			if (litNext >= 0.0)
				result = lerp(result, litNext, splitBlend);
		}
	}

	// UV rim blend into next coarser map (or to lit on far cascade edge)
	float4 scUsed = mul(shadow_matrices[used], float4(worldPos + NormalOffsetWorld(N, used), 1.0));
	float rim = saturate(CascadeUVRim(scUsed.xy) / max(rimWidth, 1e-3));
	if (rim < 0.999)
	{
		if (used + 1 < cascadeCount)
		{
			float litNext = TrySampleCascadePCF(used + 1, worldPos, N);
			if (litNext >= 0.0)
				result = lerp(litNext, result, rim);
			else
				result = lerp(1.0, result, rim);
		}
		else
		{
			result = lerp(1.0, result, smoothstep(0.0, 1.0, rim));
		}
	}

	return lerp(1.0, result, rangeFade);
}

// Fast CSM for dense detail/grass: one cascade, 4-tap PCF (no PCSS).
float SampleCSM_Fast(float3 worldPos, float3 N)
{
	const uint cascadeCount = 3;
	float3 splits = cascade_splits.xyz;
	float farSplit = max(splits.z, splits.y + 1.0);
	float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);

	float slightFade = saturate(max(dev_param_1.w, 0.2));
	float fadeStart = farSplit * lerp(0.78, 0.92, slightFade);
	float rangeFade = 1.0 - smoothstep(fadeStart, farSplit, viewZ);
	if (rangeFade <= 1e-3)
		return 1.0;

	uint primary = 2;
	if (viewZ < splits.x)
		primary = 0;
	else if (viewZ < splits.y)
		primary = 1;

	float lit = -1.0;
	[unroll]
	for (uint c = primary; c < cascadeCount; ++c)
	{
		lit = TrySampleCascadePCF(c, worldPos, N);
		if (lit >= 0.0)
			break;
	}

	if (lit < 0.0)
		return 1.0;
	return lerp(1.0, lit, rangeFade);
}

float TrySampleCascadeVolumetric(uint c, float3 worldPos)
{
	float4 sc = mul(shadow_matrices[c], float4(worldPos, 1.0));
	if (!CascadeCoordValid(sc))
		return -1.0;
	float rd = saturate(sc.z - ReceiverBias(c));
	return ShadowCmpLit(float3(sc.xy, (float)c), rd);
}

float SampleCSM_Volumetric(float3 worldPos)
{
	const uint cascadeCount = 3;
	float3 splits = cascade_splits.xyz;
	float rimWidth = cascade_splits.w > 1e-4 ? cascade_splits.w : 0.12;
	float farSplit = max(splits.z, splits.y + 1.0);
	float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);

	float slightFade = saturate(max(dev_param_1.w, 0.2));
	float fadeStart = farSplit * lerp(0.78, 0.92, slightFade);
	float rangeFade = 1.0 - smoothstep(fadeStart, farSplit, viewZ);
	if (rangeFade <= 1e-3)
		return 0.0;

	uint primary = 2;
	if (viewZ < splits.x)
		primary = 0;
	else if (viewZ < splits.y)
		primary = 1;

	float blendStart = 0.0;
	float blendEnd = 0.0;
	uint nextC = primary;
	if (primary == 0)
	{
		blendEnd = splits.x;
		blendStart = splits.x * 0.65;
		nextC = 1;
	}
	else if (primary == 1)
	{
		blendEnd = splits.y;
		blendStart = lerp(splits.x, splits.y, 0.65);
		nextC = 2;
	}

	float litPrimary = -1.0;
	uint used = primary;
	[unroll]
	for (uint c = primary; c < cascadeCount; ++c)
	{
		float lit = TrySampleCascadeVolumetric(c, worldPos);
		if (lit >= 0.0)
		{
			litPrimary = lit;
			used = c;
			break;
		}
	}

	if (litPrimary < 0.0)
		return 0.0;

	float result = litPrimary;

	if (primary < 2 && used == primary)
	{
		float splitBlend = saturate((viewZ - blendStart) / max(blendEnd - blendStart, 1e-3));
		splitBlend = smoothstep(0.0, 1.0, splitBlend);
		if (splitBlend > 1e-3)
		{
			float litNext = TrySampleCascadeVolumetric(nextC, worldPos);
			if (litNext >= 0.0)
				result = lerp(result, litNext, splitBlend);
		}
	}

	float4 scUsed = mul(shadow_matrices[used], float4(worldPos, 1.0));
	float rim = saturate(CascadeUVRim(scUsed.xy) / max(rimWidth, 1e-3));
	if (rim < 0.999)
	{
		if (used + 1 < cascadeCount)
		{
			float litNext = TrySampleCascadeVolumetric(used + 1, worldPos);
			if (litNext >= 0.0)
				result = lerp(litNext, result, rim);
			else
				result = lerp(0.0, result, rim);
		}
		else
		{
			result *= smoothstep(0.0, 1.0, rim);
		}
	}

	return result * rangeFade;
}

// Overload without a normal (volumetric march, debug): no normal-offset bias.
float SampleCSM(float3 worldPos)
{
	return SampleCSM(worldPos, float3(0.0, 0.0, 0.0));
}

// Debug overlay (r_shadow_debug -> dev_param_3.z). Returns a diagnostic color:
//   1 = selected cascade index (R=0, G=1, B=2, black = outside all cascades)
//   2 = raw SampleCSM occlusion term (grayscale; black = shadowed, white = lit)
//   3 = cascade UV.xy in RG, projected depth in B (should be a smooth ramp)
//   4 = N.L against the sun (grayscale; verifies sun direction independent of maps)
float3 ShadowDebugColor(float3 worldPos, float3 N, float3 L, int mode)
{
	int used = -1;
	float4 scUsed = float4(0, 0, 0, 0);
	[unroll]
	for (uint c = 0; c < 3; ++c)
	{
		float4 sc = mul(shadow_matrices[c], float4(worldPos, 1.0));
		if (CascadeCoordValid(sc)) { used = int(c); scUsed = sc; break; }
	}

	if (mode == 1)
	{
		if (used == 0) return float3(1, 0, 0);
		if (used == 1) return float3(0, 1, 0);
		if (used == 2) return float3(0, 0, 1);
		return float3(0, 0, 0);
	}
	if (mode == 2)
	{
		float s = SampleCSM(worldPos, N);
		return float3(s, s, s);
	}
	if (mode == 3)
	{
		if (used < 0) return float3(0, 0, 0);
		return float3(saturate(scUsed.xy), saturate(scUsed.z));
	}
	// mode == 4
	float ndl = saturate(dot(normalize(N), normalize(L)));
	return float3(ndl, ndl, ndl);
}

#endif // CSM_SHADOW_FORWARD

#ifndef CSM_SHADOW_FORWARD
float SampleCSM(float3 worldPos, float3 N)
{
	return 1.0;
}

float SampleCSM(float3 worldPos)
{
	return 1.0;
}

float SampleCSM_Fast(float3 worldPos, float3 N)
{
	return 1.0;
}

float SampleCSM_Volumetric(float3 worldPos)
{
	return 1.0;
}

float3 ShadowDebugColor(float3 worldPos, float3 N, float3 L, int mode)
{
	float ndl = saturate(dot(normalize(N), normalize(L)));
	return float3(ndl, ndl, ndl);
}
#endif

float SampleHUDShadow(float3 worldPos)
{
#ifndef HUD_SHADOW_FORWARD
	return 1.0;
#else
	// shadow_matrices[3] = dedicated HUD light-VP (clip→UV)
	float4 shadowCoord = mul(shadow_matrices[3], float4(worldPos, 1.0));
	float2 shadowUV = shadowCoord.xy;
	float depth = shadowCoord.z;

	if (any(shadowUV < 0.0) || any(shadowUV > 1.0))
		return 1.0;

	float hudSmapSize = 1024.0;
	float ts = 0.6 / hudSmapSize;
	float lit = 0.0;
	lit += g_HUDShadowMap.SampleCmp(smp_shadowcmp, float3(shadowUV + float2(-ts, -ts), 0), depth);
	lit += g_HUDShadowMap.SampleCmp(smp_shadowcmp, float3(shadowUV + float2(+ts, -ts), 0), depth);
	lit += g_HUDShadowMap.SampleCmp(smp_shadowcmp, float3(shadowUV + float2(-ts, +ts), 0), depth);
	lit += g_HUDShadowMap.SampleCmp(smp_shadowcmp, float3(shadowUV + float2(+ts, +ts), 0), depth);
	return lit * 0.25;
#endif
}

// Spot / OMNIPART local shadows.
// localShadowRect.w = (page + 1) + opacity * 0.999  (opacity = temporal fade).
// spotVP is clip-space; UV uses Y-flip to match D3D→Vulkan (NVRHI) viewport.
// softKernel = r2_ls_psm_kernel / r2_ls_ssm_kernel (classic KERNEL texels, ~0.6–0.7).
// Atlas res in dev_param_4.w (r2_smap_size × r2_ls_squality).
float SampleLocalShadow(float3 worldPos, float3 N, float4x4 spotVP, float4 shadowRect, float softKernel)
{
#ifndef LOCAL_SHADOW_FORWARD
	return 1.0;
#else
	float pagePlus = shadowRect.w;
	if (pagePlus < 0.5)
		return 1.0;

	float opacity = frac(pagePlus);
	if (opacity < 1e-3)
		return 1.0;

	float scale = shadowRect.z;
	if (scale < 1e-4)
		return 1.0;

	float3 wp = worldPos + normalize(N) * 0.001;

	float4 sc = mul(spotVP, float4(wp, 1.0));
	if (sc.w <= 1e-4)
		return 1.0;

	float3 ndc = sc.xyz / sc.w;
	float2 lightUV = ndc.xy * 0.5 + 0.5;
	lightUV.y = 1.0 - lightUV.y;
	float depth = ndc.z;

	if (any(lightUV < 0.0) || any(lightUV > 1.0) || depth < 0.0 || depth > 1.0)
		return 1.0;

	float2 uv = lightUV * scale + shadowRect.xy;
	uint page = uint(pagePlus) - 1;

	float atlas = max(floor(dev_param_4.w), 512.0);
	bool useEsm = frac(dev_param_4.w) > 0.25;
	float kernel = max(softKernel, 0.1);
	float ts = kernel / atlas;
	float2 uvMin = shadowRect.xy + (0.5 / atlas);
	float2 uvMax = shadowRect.xy + scale - (0.5 / atlas);
	float lit = 0.0;

	if (useEsm)
	{
		const float k = 80.0;
		float2 o[4] = { float2(-ts, -ts), float2(+ts, -ts), float2(-ts, +ts), float2(+ts, +ts) };
		[unroll] for (int i = 0; i < 4; ++i)
		{
			float2 suv = clamp(uv + o[i], uvMin, uvMax);
			float occluder = g_LocalShadowESM.SampleLevel(smp_shadow_point, float3(suv, page), 0).x;
			float receiver = exp(k * saturate(depth));
			lit += saturate(receiver / max(occluder, 1e-4));
		}
		lit *= 0.25;
	}
	else
	{
		float depthBias = max(abs(dev_param_2.x), 0.00005);
		float rd = saturate(depth - depthBias);
		lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(clamp(uv + float2(-ts, -ts), uvMin, uvMax), page), rd);
		lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(clamp(uv + float2(+ts, -ts), uvMin, uvMax), page), rd);
		lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(clamp(uv + float2(-ts, +ts), uvMin, uvMax), page), rd);
		lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(clamp(uv + float2(+ts, +ts), uvMin, uvMax), page), rd);
		lit *= 0.25;
	}

	float2 edge = min(lightUV, 1.0 - lightUV);
	float rim = smoothstep(0.0, 0.012, min(edge.x, edge.y));
	return lerp(1.0, lit, rim * opacity);
#endif
}

float SampleLocalShadow(float3 worldPos, float3 N, float4x4 spotVP, float4 shadowRect)
{
	float soft = max(max(dev_param_1.y, dev_param_1.z), 0.6);
	return SampleLocalShadow(worldPos, N, spotVP, shadowRect, soft);
}

float SampleLocalShadow(float3 worldPos, float4x4 spotVP, float4 shadowRect)
{
	return SampleLocalShadow(worldPos, float3(0, 1, 0), spotVP, shadowRect);
}

#endif
