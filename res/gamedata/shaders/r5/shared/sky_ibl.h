#ifndef SKY_IBL_H
#define SKY_IBL_H

#ifndef SKY_IBL_CUBES_DECLARED
TextureCube s_env0 : register(t25);
TextureCube s_env1 : register(t26);
#define SKY_IBL_CUBES_DECLARED
#endif

#ifndef SKY_IBL_SPLITSUM_DECLARED
Texture2D s_envBRDF : register(t52);
StructuredBuffer<float4> g_IBLSkySH : register(t53);
StructuredBuffer<float4> g_IBLProbes : register(t54);
TextureCubeArray s_probeSpec : register(t55);
#define SKY_IBL_SPLITSUM_DECLARED
#endif

static const uint IBL_SH_COUNT = 9;
static const uint IBL_PROBE_STRIDE = 16;
static const uint IBL_MAX_PROBES = 16;

float3 SampleSkyRGB(float3 dir)
{
	float3 d = normalize(dir);
	float3 s0 = s_env0.SampleLevel(smp_rtlinear, d, 0.0).rgb;
	float3 s1 = s_env1.SampleLevel(smp_rtlinear, d, 0.0).rgb;
	return lerp(s0, s1, saturate(L_ambient.w));
}

float3 EvalSH9(StructuredBuffer<float4> sh, uint base, float3 n)
{
	float x = n.x;
	float y = n.y;
	float z = n.z;
	float3 c = 0.0.xxx;
	c += sh[base + 0].xyz * 0.282095;
	c += sh[base + 1].xyz * (-0.488603 * y);
	c += sh[base + 2].xyz * (0.488603 * z);
	c += sh[base + 3].xyz * (-0.488603 * x);
	c += sh[base + 4].xyz * (1.092548 * x * y);
	c += sh[base + 5].xyz * (-1.092548 * y * z);
	c += sh[base + 6].xyz * (0.315392 * (3.0 * z * z - 1.0));
	c += sh[base + 7].xyz * (-1.092548 * x * z);
	c += sh[base + 8].xyz * (0.546274 * (x * x - y * y));
	return max(c, 0.0.xxx);
}

float3 SampleSkyIrradianceSH(float3 N)
{
	float3 n = normalize(N);
	float3 a = EvalSH9(g_IBLSkySH, 0, n);
	float3 b = EvalSH9(g_IBLSkySH, IBL_SH_COUNT, n);
	return lerp(a, b, saturate(L_ambient.w));
}

float RoughnessToMip(float roughness, float mipCount)
{
	return saturate(roughness) * max(mipCount - 1.0, 0.0);
}

float3 SampleSkySpecularPrefiltered(float3 R, float roughness, float mipCount)
{
	float mip = RoughnessToMip(roughness, mipCount);
	float3 d = normalize(R);
	float3 s0 = s_env0.SampleLevel(smp_rtlinear, d, mip).rgb;
	float3 s1 = s_env1.SampleLevel(smp_rtlinear, d, mip).rgb;
	return lerp(s0, s1, saturate(L_ambient.w));
}

float3 ParallaxCorrectDir(float3 R, float3 worldPos, float3 boxMin, float3 boxMax, float3 probePos)
{
	float3 first = (boxMax - worldPos) / max(R, float3(1e-4, 1e-4, 1e-4));
	float3 second = (boxMin - worldPos) / min(R, float3(-1e-4, -1e-4, -1e-4));
	float3 further = max(first, second);
	float dist = min(further.x, min(further.y, further.z));
	float3 hit = worldPos + R * max(dist, 0.0);
	return normalize(hit - probePos);
}

void SampleLocalProbe(
	float3 worldPos,
	float3 N,
	float3 R,
	float roughness,
	float mipCount,
	inout float3 irradiance,
	inout float3 specular,
	inout float probeWeight)
{
	probeWeight = 0.0;
	uint count = min((uint)g_IBLProbes[0].w, IBL_MAX_PROBES);
	if (count == 0)
		return;

	float3 accIrr = 0.0.xxx;
	float3 accSpec = 0.0.xxx;
	float wSum = 0.0;
	float mip = RoughnessToMip(roughness, mipCount);
	float3 n = normalize(N);

	[loop] for (uint i = 0; i < count; ++i)
	{
		uint base = 1 + i * IBL_PROBE_STRIDE;
		float3 pos = g_IBLProbes[base + 0].xyz;
		float radius = max(g_IBLProbes[base + 0].w, 0.01);
		float3 bmin = g_IBLProbes[base + 1].xyz;
		float3 bmax = g_IBLProbes[base + 2].xyz;
		float intensity = max(g_IBLProbes[base + 2].w, 0.0);
		float d = length(worldPos - pos);
		float w = saturate(1.0 - d / radius);
		w = w * w * (3.0 - 2.0 * w);
		w *= intensity;
		if (any(worldPos < bmin) || any(worldPos > bmax))
			w *= 0.35;
		if (w < 1e-3)
			continue;

		float3 pr = ParallaxCorrectDir(R, worldPos, bmin, bmax, pos);
		float3 spec = s_probeSpec.SampleLevel(smp_rtlinear, float4(pr, (float)i), mip).rgb;
		float3 irr = EvalSH9(g_IBLProbes, base + 3, n);
		if (dot(irr, 1.0.xxx) < 1e-4)
			irr = SampleSkyIrradianceSH(n) * 0.35;

		accIrr += irr * w;
		accSpec += spec * w;
		wSum += w;
	}

	if (wSum < 1e-3)
		return;

	float influence = saturate(wSum);
	irradiance = lerp(irradiance, accIrr / wSum, influence);
	specular = lerp(specular, accSpec / wSum, influence);
	probeWeight = influence;
}

float3 SoftSky(float3 c)
{
	float3 x = max(c, 0.0);
	float lum = dot(x, float3(0.2126, 0.7152, 0.0722));
	return x * rcp(1.0 + max(lum, 0.0) * 0.65);
}

float3 PBRAmbientIBL(
	float3 albedo,
	float3 N,
	float3 V,
	float3 worldPos,
	float metallic,
	float roughness,
	float ao,
	float hemiOcclusion,
	float intensity)
{
	float3 F0 = CalculateF0(albedo, metallic);
	float NdotV = max(dot(N, V), 0.0);
	float3 F = F_SchlickRoughness(NdotV, F0, roughness);
	float hemiDir = 0.55 + 0.45 * saturate(N.y);
	float3 weatherScale = max(L_ambient.rgb + L_hemi_color.rgb * L_hemi_color.w * hemiDir, 0.0);

	float4 iblMeta = g_IBLSkySH[IBL_SH_COUNT * 2];
	float mipCount = max(iblMeta.x, 1.0);
	float prefilterBlend = saturate(iblMeta.y);
	float ssgiScale = saturate(iblMeta.z);

	float3 R = reflect(-V, N);
	float lodMix = saturate(roughness * roughness);
	float3 roughDir = normalize(lerp(R, float3(R.x, abs(R.y) + lodMix * 0.35, R.z), lodMix));
	float3 irrRaw = SampleSkyRGB(N);
	float3 specRaw = SampleSkyRGB(roughDir);
	if (lodMix > 0.05)
		specRaw = lerp(specRaw, SampleSkyRGB(N), lodMix);

	float3 irrPF = SampleSkyIrradianceSH(N);
	float3 specPF = SampleSkySpecularPrefiltered(R, roughness, mipCount);
	float probeW = 0.0;
	SampleLocalProbe(worldPos, N, R, roughness, mipCount, irrPF, specPF, probeW);

	float3 irradiance = SoftSky(lerp(irrRaw, irrPF, max(prefilterBlend, 0.65)));
	float3 specEnv = SoftSky(lerp(specRaw, specPF, prefilterBlend));

	float weatherLum = max(dot(weatherScale, float3(0.2126, 0.7152, 0.0722)), 1e-4);
	float irrLum = max(dot(max(irradiance, 0.0), float3(0.2126, 0.7152, 0.0722)), 1e-4);
	float weatherChroma = length(weatherScale - weatherLum.xxx);
	float overcast = saturate(1.0 - weatherChroma * 6.0) * saturate(weatherLum * 1.1);

	float match = weatherLum / irrLum;
	match = clamp(match, 0.75, 1.6);
	irradiance *= match * max(intensity, 0.35) * max(ssgiScale, 0.65);
	irradiance = max(irradiance, weatherScale * lerp(0.85, 0.7, overcast));

	float roughSpec = saturate(1.05 - roughness * 1.15);
	specEnv *= weatherScale * max(intensity, 0.35) * roughSpec;

	float3 kD = (1.0 - F) * (1.0 - metallic);
	float wetComp = saturate((0.4 - roughness) * 2.5);
	float3 diffuseAmbient = kD * albedo * irradiance * (1.0 + wetComp * 0.45);

	float2 lutApprox = EnvBRDFApprox(NdotV, roughness);
	float2 lutSplit = s_envBRDF.SampleLevel(smp_rtlinear, float2(NdotV, saturate(roughness)), 0).rg;
	float2 lut = lerp(lutApprox, lutSplit, prefilterBlend);
	float3 specularAmbient = EnvBRDF(F0, NdotV, roughness, lut) * specEnv;
	specularAmbient *= roughSpec;

	return (diffuseAmbient + specularAmbient) * ao * saturate(hemiOcclusion);
}

#endif
