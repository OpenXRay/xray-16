#ifndef RESTIR_DI_EVAL_H
#define RESTIR_DI_EVAL_H

struct GPULightDataDI {
    float4 positionAndInvRangeSq;
    float4 colorAndRange;
    float4 directionAndSpotScale;
    float4 spotParamsAndType;
    float4x4 spotVP;
};

float PointAttenDI(float distSq, float invRangeSq)
{
    return saturate(1.0 - distSq * abs(invRangeSq));
}

float SpotAttenDI(float3 toLight, float3 spotDir, float scale, float offset)
{
    float cosAngle = dot(normalize(-toLight), spotDir);
    return saturate(cosAngle * scale + offset);
}

bool IsLiveLightDI(GPULightDataDI light)
{
    return light.colorAndRange.w > 1e-4 && abs(light.positionAndInvRangeSq.w) > 1e-8;
}

float EvalLocalLightAttenuationDI(GPULightDataDI light, float3 worldPos, out float3 L, out float dist, out float3 lightColor)
{
    L = float3(0, 0, 1);
    dist = 0;
    lightColor = 0;
    if (!IsLiveLightDI(light))
        return 0;

    float3 lightPos = light.positionAndInvRangeSq.xyz;
    float invRangeSq = abs(light.positionAndInvRangeSq.w);
    lightColor = light.colorAndRange.xyz;
    float lightType = light.spotParamsAndType.y;

    float3 toLight = lightPos - worldPos;
    float distSq = dot(toLight, toLight);
    dist = sqrt(max(distSq, 1e-8));
    L = toLight / dist;
    float atten = PointAttenDI(distSq, invRangeSq);

    if (lightType > 0.5) {
        uint texIdx = asuint(light.spotParamsAndType.z);
        if (texIdx != 0xFFFFFFFFu) {
            float4 projPos = mul(light.spotVP, float4(worldPos, 1.0));
            float3 spotDir = light.directionAndSpotScale.xyz;
            float spotScale = light.directionAndSpotScale.w;
            float spotOffset = light.spotParamsAndType.x;
            float cone = SpotAttenDI(toLight, spotDir, spotScale, spotOffset);
            if (projPos.w > 1e-4) {
                float2 projUV = projPos.xy / projPos.w * 0.5 + 0.5;
                projUV.y = 1.0 - projUV.y;
                if (all(projUV >= 0.0) && all(projUV <= 1.0)) {
                    Texture2D spotTex = GetBindlessTexture(texIdx);
                    atten *= spotTex.SampleLevel(smp_linear, projUV, 0).r * cone;
                } else {
                    atten = 0;
                }
            } else {
                atten = 0;
            }
        } else {
            float3 spotDir = light.directionAndSpotScale.xyz;
            float spotScale = light.directionAndSpotScale.w;
            float spotOffset = light.spotParamsAndType.x;
            atten *= SpotAttenDI(toLight, spotDir, spotScale, spotOffset);
        }
    }
    return atten;
}

float EvalLocalLightTargetPdfDI(
    GPULightDataDI light, float3 worldPos, float3 N, float3 V,
    float3 albedo, float metallic, float roughness)
{
    float3 L, lightColor;
    float dist;
    float atten = EvalLocalLightAttenuationDI(light, worldPos, L, dist, lightColor);
    if (atten <= 0.001)
        return 0;
    float3 lit = PBRDirectLighting(albedo, N, V, L, lightColor * atten, metallic, roughness, 1);
    return Luminance(lit);
}

bool IsHudLightDI(GPULightDataDI light)
{
    return light.positionAndInvRangeSq.w < 0.0;
}

bool IsTransientPointLightDI(GPULightDataDI light)
{
    return light.spotParamsAndType.y < 0.5 && light.spotParamsAndType.x > 0.5;
}

float LightEmitterRadiusDI(GPULightDataDI light)
{
    float range = max(abs(light.colorAndRange.w), 0.5);
    return clamp(range * 0.02, 0.03, 0.4);
}

float SoftShadowAmountDI(float dist)
{
    return saturate((dist - 0.35) / 6.0);
}

float LightEmitterRadiusDISoft(GPULightDataDI light)
{
    float range = max(abs(light.colorAndRange.w), 0.5);
    return clamp(range * 0.035, 0.05, 0.65);
}

#endif
