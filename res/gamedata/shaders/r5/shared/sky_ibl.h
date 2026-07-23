// sky_ibl.h — IX-Ray-style sky cubemap IBL (fake irradiance + rough specular)
#ifndef SKY_IBL_H
#define SKY_IBL_H

#ifndef SKY_IBL_CUBES_DECLARED
TextureCube s_env0 : register(t25);
TextureCube s_env1 : register(t26);
#define SKY_IBL_CUBES_DECLARED
#endif

// L_ambient.w = skybox lerp; parallax.w = IBL intensity (0 = classic ambient)
float3 SampleSkyRGB(float3 dir)
{
    float3 d = normalize(dir);
    float3 s0 = s_env0.SampleLevel(smp_rtlinear, d, 0.0).rgb;
    float3 s1 = s_env1.SampleLevel(smp_rtlinear, d, 0.0).rgb;
    return lerp(s0, s1, saturate(L_ambient.w));
}

// Fake irradiance: N + hemisphere taps (cubes often lack useful mips)
float3 SampleSkyIrradiance(float3 N)
{
    N = normalize(N);
    // Orthonormal basis around N
    float3 up = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);

    float3 irr = SampleSkyRGB(N) * 0.30;
    irr += SampleSkyRGB(normalize(N + T * 0.55)) * 0.14;
    irr += SampleSkyRGB(normalize(N - T * 0.55)) * 0.14;
    irr += SampleSkyRGB(normalize(N + B * 0.55)) * 0.14;
    irr += SampleSkyRGB(normalize(N - B * 0.55)) * 0.14;
    irr += SampleSkyRGB(normalize(N + T * 0.35 + B * 0.35)) * 0.07;
    irr += SampleSkyRGB(normalize(N - T * 0.35 + B * 0.35)) * 0.07;
    return irr;
}

float3 SampleSkySpecular(float3 R, float roughness)
{
    // Fake LOD: bias sample direction toward N for rough materials
    float lodMix = saturate(roughness * roughness);
    // Remap: keep some sky color even when rough (IBL_FAKE_IRRADANCE style)
    float3 dir = normalize(lerp(R, float3(R.x, abs(R.y) + lodMix * 0.35, R.z), lodMix));
    float3 spec = SampleSkyRGB(dir);
    // Soften with irradiance for rough
    if (lodMix > 0.05)
        spec = lerp(spec, SampleSkyIrradiance(dir), lodMix);
    return spec;
}

float3 PBRAmbientIBL(
    float3 albedo,
    float3 N,
    float3 V,
    float metallic,
    float roughness,
    float ao,
    float hemiOcclusion,
    float intensity)
{
    float3 F0 = CalculateF0(albedo, metallic);
    float NdotV = max(dot(N, V), 0.0);
    float3 F = F_SchlickRoughness(NdotV, F0, roughness);

    float3 irradiance = SampleSkyIrradiance(N);
    // Weather tint / night scale from classic constants
    float3 weatherScale = L_ambient.rgb + L_hemi_color.rgb * L_hemi_color.w;
    irradiance *= weatherScale * intensity;

    float3 kD = (1.0 - F) * (1.0 - metallic);
    float3 diffuseAmbient = kD * albedo * irradiance;

    float3 R = reflect(-V, N);
    float3 specEnv = SampleSkySpecular(R, roughness);
    specEnv *= weatherScale * intensity;
    // Specular ambient strength scales with gloss
    float3 specularAmbient = F * specEnv * (0.25 + 0.75 * (1.0 - roughness));

    return (diffuseAmbient + specularAmbient) * ao * saturate(hemiOcclusion);
}

#endif // SKY_IBL_H
