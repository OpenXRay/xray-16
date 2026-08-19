#ifndef ATMOSPHERE_H
#define ATMOSPHERE_H

static const float3 ATM_RAYLEIGH = float3(5.8e-3, 1.35e-2, 3.31e-2);
static const float ATM_MIE = 2.1e-2;
static const float ATM_MIE_G = 0.76;

float AtmosphereRayleighPhase(float cosTheta)
{
    return 0.0596831 * (1.0 + cosTheta * cosTheta);
}

float AtmosphereMiePhase(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return 0.1193662 * (1.0 - g2) / max(pow(max(denom, 1e-4), 1.5), 1e-4);
}

void AtmosphereAerial(float3 viewDir, float dist, float3 sunDir, float3 sunColor, float3 fogColor, float strength,
    out float3 transmittance, out float3 inscatter)
{
    float t = max(dist, 0.0);
    float height = saturate(viewDir.y * 0.5 + 0.5);
    float density = exp(-height * 2.4) * strength;
    float3 betaR = ATM_RAYLEIGH * density;
    float betaM = ATM_MIE * density;
    float3 ext = betaR + betaM;
    transmittance = exp(-ext * t);
    float cosTheta = saturate(dot(viewDir, sunDir));
    float3 ray = betaR * AtmosphereRayleighPhase(cosTheta);
    float mie = betaM * AtmosphereMiePhase(cosTheta, ATM_MIE_G);
    float3 scatter = (ray + mie) * sunColor + fogColor * betaR * 0.18;
    float3 invExt = 1.0 / max(ext, 1e-4);
    inscatter = scatter * invExt * (1.0 - transmittance);
}

float3 AtmosphereSkyInscatter(float3 viewDir, float3 sunDir, float3 sunColor, float3 fogColor, float3 skyTint, float strength)
{
    float3 T, insc;
    AtmosphereAerial(viewDir, 80.0, sunDir, sunColor, fogColor, strength, T, insc);
    float horizon = saturate(1.0 - abs(viewDir.y));
    float sunGlow = pow(saturate(dot(viewDir, sunDir)), 32.0);
    return (insc * skyTint + sunColor * sunGlow * 0.12 * horizon) * strength;
}

#endif
