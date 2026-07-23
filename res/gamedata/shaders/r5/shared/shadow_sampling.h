// shared/shadow_sampling.h — CSM + soft PCSS (FrameGraph r5)
#ifndef SHADOW_SAMPLING_H
#define SHADOW_SAMPLING_H

#ifdef CSM_SHADOW_FORWARD
// Per-cascade maps (resolution ladder). Texture2D — not Array: one SampleCmp
// target per cascade avoids layout/slot mismatches with a shared array.
Texture2D<float> g_ShadowMap0 : register(t23);
Texture2D<float> g_ShadowMap1 : register(t29);
Texture2D<float> g_ShadowMap2 : register(t30);
SamplerComparisonState smp_shadowcmp : register(s4);
SamplerState smp_shadow_point : register(s5);
#endif

#ifdef HUD_SHADOW_FORWARD
Texture2DArray<float> g_HUDShadowMap : register(t24);
#endif

#ifdef LOCAL_SHADOW_FORWARD
// Spot / OMNIPART local shadow atlas (t27 — free after g_ContactDepth removal)
Texture2DArray<float> g_LocalShadowAtlas : register(t27);
#ifndef CSM_SHADOW_FORWARD
SamplerComparisonState smp_shadowcmp : register(s4);
#endif
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

float Gaussian11(int i)
{
	static const float k[11] = {
		0.0093, 0.0280, 0.0650, 0.1210, 0.1760, 0.2000,
		0.1760, 0.1210, 0.0650, 0.0280, 0.0093
	};
	return k[i + 5];
}

int BlockerKernelRadius(float sunQuality)
{
	return (sunQuality >= 2.5) ? 5 : (sunQuality >= 1.5) ? 3 : 2;
}

int FilterKernelRadius(float sunQuality)
{
	// Default tier (quality 1) uses radius 3: radius 2 (5 taps) left penumbrae too
	// crisp — with more taps the variable-width PCSS filter reads as genuinely soft.
	return (sunQuality >= 2.5) ? 5 : (sunQuality >= 1.5) ? 4 : 3;
}

float SampleCascadePCF(uint cascade, float2 shadowUV, float refDepth, float softScale)
{
	float smapSize = CascadeSmapSize(cascade);
	float ts = (0.6 * max(softScale, 1.0)) / smapSize;
	float rd = saturate(refDepth - ReceiverBias(cascade));
	float lit = 0.0;
	lit += ShadowCmpLit(float3(shadowUV + float2(-ts, -ts), (float)cascade), rd);
	lit += ShadowCmpLit(float3(shadowUV + float2(+ts, -ts), (float)cascade), rd);
	lit += ShadowCmpLit(float3(shadowUV + float2(-ts, +ts), (float)cascade), rd);
	lit += ShadowCmpLit(float3(shadowUV + float2(+ts, +ts), (float)cascade), rd);
	return lit * 0.25;
}

float FindAverageBlockerDepth(
	uint cascade,
	float2 shadowUV,
	float refDepth,
	float searchRadiusUV,
	int kernelRadius,
	out float blockerCount)
{
	float avgBlocker = 0.0;
	blockerCount = 0.0;
	int R = clamp(kernelRadius, 1, 5);

	[unroll]
	for (int y = -5; y <= 5; ++y)
	{
		if (abs(y) > R)
			continue;
		[unroll]
		for (int x = -5; x <= 5; ++x)
		{
			if (abs(x) > R)
				continue;

			float2 uv = shadowUV + float2((float)x, (float)y) * searchRadiusUV;
			float d = ShadowDepth(cascade, uv);
			if (d < refDepth - ReceiverBias(cascade) - 1e-5)
			{
				float w = Gaussian11(x) * Gaussian11(y);
				avgBlocker += d * w;
				blockerCount += w;
			}
		}
	}

	if (blockerCount > 1e-5)
		return avgBlocker / blockerCount;
	return 0.0;
}

float FilterPCSS(
	uint cascade,
	float2 shadowUV,
	float refDepth,
	float filterRadiusUV,
	int kernelRadius)
{
	float sum = 0.0;
	float wsum = 0.0;
	float rd = saturate(refDepth - ReceiverBias(cascade));
	int R = clamp(kernelRadius, 1, 5);

	[unroll]
	for (int y = -5; y <= 5; ++y)
	{
		if (abs(y) > R)
			continue;
		[unroll]
		for (int x = -5; x <= 5; ++x)
		{
			if (abs(x) > R)
				continue;

			float w = Gaussian11(x) * Gaussian11(y);
			float2 uv = shadowUV + float2((float)x, (float)y) * filterRadiusUV;
			sum += w * ShadowCmpLit(float3(uv, (float)cascade), rd);
			wsum += w;
		}
	}

	return (wsum > 1e-5) ? (sum / wsum) : 1.0;
}

// Full PCSS: blocker search + penumbra estimate + variable-size weighted filter.
// sunQuality in dev_param_3.y (r2_sun_quality 0..3). Tunables in dev_param_4:
//   .x = r2_sun_soft    (max penumbra, texels)  — how soft shadows get far from the occluder
//   .y = r2_sun_blocker (blocker-search spacing, texels)
//   .z = r2_sun_contact (min penumbra, texels)  — contact sharpness
float SampleCascadePCSS(uint cascade, float2 shadowUV, float refDepth, float viewZ, float midSplit)
{
	float smapSize = CascadeSmapSize(cascade);
	float sunQuality = dev_param_3.y;

	float softTexels    = dev_param_4.x > 0.0 ? dev_param_4.x : 6.0;
	float blockerTexels = dev_param_4.y > 0.0 ? dev_param_4.y : 2.5;
	float contactTexels = dev_param_4.z > 0.0 ? dev_param_4.z : 1.0;

	// Lowest tier: a single crisp contact-sized tap.
	if (sunQuality < 0.5)
		return SampleCascadePCF(cascade, shadowUV, refDepth, contactTexels / 0.6);

	int blockerRadius = BlockerKernelRadius(sunQuality);
	int filterRadius = FilterKernelRadius(sunQuality);

	float blockerSearchUV = max(blockerTexels, 0.5) / smapSize;
	float blockerCount = 0.0;
	float avgBlocker = FindAverageBlockerDepth(
		cascade, shadowUV, refDepth, blockerSearchUV, blockerRadius, blockerCount);

	// No occluder standing above this receiver → sharp contact tap, no false blur.
	if (blockerCount <= 0.12)
		return SampleCascadePCF(cascade, shadowUV, refDepth, contactTexels / 0.6);

	// PCSS penumbra grows with the receiver→blocker gap. The gap is in normalized
	// cascade depth (~0.1-0.4 for real occluders), so keep the gain low: a high gain
	// saturates penumbra to max for almost everything, widening the filter so much that
	// thin shadows (tree trunks/branches) only cover the center tap and wash out to lit.
	float gap = max(refDepth - avgBlocker, 0.0);
	float penumbra = saturate(gap / max(avgBlocker, 1e-3) * 3.0);

	float cascadeScale = 1.0 + float(cascade) * 0.3;
	float maxTexels = softTexels * cascadeScale;
	float widthTexels = clamp(lerp(contactTexels, maxTexels, penumbra), contactTexels, maxTexels);

	// widthTexels is the half-extent of the kernel; convert to per-tap spacing so the
	// filter always spans ±widthTexels regardless of tap count (smooth, no aliasing).
	float filterRadiusUV = (widthTexels / float(max(filterRadius, 1))) / smapSize;
	return FilterPCSS(cascade, shadowUV, refDepth, filterRadiusUV, filterRadius);
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

	// Soft end of shadow distance — keep most of r2_sun_far usable (was 0.72→too early).
	float rangeFade = 1.0 - smoothstep(farSplit * 0.88, farSplit, viewZ);
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
	for (uint c = 0; c < cascadeCount; ++c)
	{
		if (c < primary)
			continue;
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

	float rangeFade = 1.0 - smoothstep(farSplit * 0.88, farSplit, viewZ);
	if (rangeFade <= 1e-3)
		return 1.0;

	uint primary = 2;
	if (viewZ < splits.x)
		primary = 0;
	else if (viewZ < splits.y)
		primary = 1;

	float lit = -1.0;
	[unroll]
	for (uint c = 0; c < cascadeCount; ++c)
	{
		if (c < primary)
			continue;
		lit = TrySampleCascadePCF(c, worldPos, N);
		if (lit >= 0.0)
			break;
	}

	if (lit < 0.0)
		return 1.0;
	return lerp(1.0, lit, rangeFade);
}

// Cheap single-tap CSM for volumetrics. Sunshafts ray-march this 20-40× per pixel at
// full res; the PCSS path (blocker search + PCF kernel) there is the dominant cost and
// its soft penumbra is invisible in scattered light. One hardware comparison per probe.
float SampleCSM_Volumetric(float3 worldPos)
{
	const uint cascadeCount = 3;
	float3 splits = cascade_splits.xyz;
	float farSplit = max(splits.z, splits.y + 1.0);
	float viewZ = abs(mul(m_V, float4(worldPos, 1.0)).z);

	float rangeFade = 1.0 - smoothstep(farSplit * 0.88, farSplit, viewZ);
	if (rangeFade <= 1e-3)
		return 1.0;

	uint primary = 2;
	if (viewZ < splits.x)
		primary = 0;
	else if (viewZ < splits.y)
		primary = 1;

	[unroll]
	for (uint c = 0; c < cascadeCount; ++c)
	{
		if (c < primary)
			continue;
		float4 sc = mul(shadow_matrices[c], float4(worldPos, 1.0));
		if (!CascadeCoordValid(sc))
			continue;
		float rd = saturate(sc.z - ReceiverBias(c));
		float lit = ShadowCmpLit(float3(sc.xy, (float)c), rd);
		return lerp(1.0, lit, rangeFade);
	}
	return 1.0;
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

// Spot / OMNIPART local shadows. tilePlusOne = asuint(spotParamsAndType.w):
// 0 = no atlas tile, else slice = tilePlusOne - 1. spotVP is clip-space (same as
// projector sampling); UV uses Y-flip to match D3D→Vulkan viewport.
float SampleLocalShadow(float3 worldPos, float4x4 spotVP, uint tilePlusOne)
{
#ifndef LOCAL_SHADOW_FORWARD
	return 1.0;
#else
	if (tilePlusOne == 0)
		return 1.0;

	float4 sc = mul(spotVP, float4(worldPos, 1.0));
	if (sc.w <= 1e-4)
		return 1.0;

	float3 ndc = sc.xyz / sc.w;
	float2 uv = ndc.xy * 0.5 + 0.5;
	uv.y = 1.0 - uv.y;
	float depth = ndc.z;

	if (any(uv < 0.0) || any(uv > 1.0) || depth < 0.0 || depth > 1.0)
		return 1.0;

	// Classic KERNEL 0.6 4-tap PCF. Soft UV rim only (like accum_sun_far edge fade) —
	// hard [0,1] reject made shadows pop on/off right in front of the camera as the
	// light frustum swept the floor. Keep rim narrow so long mid-frustum shadows stay.
	uint slice = tilePlusOne - 1;
	float ts = 0.6 / 1024.0;
	float rd = saturate(depth - 0.0003);
	float lit = 0.0;
	lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(uv + float2(-ts, -ts), slice), rd);
	lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(uv + float2(+ts, -ts), slice), rd);
	lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(uv + float2(-ts, +ts), slice), rd);
	lit += g_LocalShadowAtlas.SampleCmp(smp_shadowcmp, float3(uv + float2(+ts, +ts), slice), rd);
	lit *= 0.25;

	float2 edge = min(uv, 1.0 - uv);
	float rim = smoothstep(0.0, 0.05, min(edge.x, edge.y));
	return lerp(1.0, lit, rim);
#endif
}

#endif
