#ifndef RTGI_RECONSTRUCT_COMMON_H
#define RTGI_RECONSTRUCT_COMMON_H

static const float RTGI_RECON_FACTOR_FLOOR = 0.001;
static const float RTGI_RECON_DIELECTRIC_F0 = 0.04;
static const float RTGI_RECON_STORAGE_LIMIT = 32768.0;
static const float RTGI_RECON_FAST_ALPHA = 0.25;
static const float RTGI_RECON_CHANGE_SIGMA = 2.0;
static const float RTGI_RECON_CHANGE_EPSILON = 0.02;
static const float RTGI_RECON_CHANGE_HISTORY = 4.0;
static const float RTGI_RECON_VARIANCE_HISTORY = 4.0;
static const float RTGI_RECON_ROUGHNESS_SCALE = 16.0;
static const float RTGI_RECON_WEIGHT_EPSILON = 1e-4;

static const uint RTGI_RECON_REJECT_NONE = 0u;
static const uint RTGI_RECON_REJECT_NO_HISTORY = 1u;
static const uint RTGI_RECON_REJECT_MOTION = 2u;
static const uint RTGI_RECON_REJECT_OFFSCREEN = 3u;
static const uint RTGI_RECON_REJECT_GEOMETRY = 4u;
static const uint RTGI_RECON_REJECT_NORMAL = 5u;
static const uint RTGI_RECON_REJECT_HUD = 6u;
static const uint RTGI_RECON_REJECT_PRIMARY = 7u;
static const float RTGI_RECON_REJECT_SCALE = 0.125;

float RTGIReconLuminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 RTGIReconDiffuseFactor(float4 albedoMetallic)
{
    return max(albedoMetallic.rgb * (1.0 - saturate(albedoMetallic.a)), RTGI_RECON_FACTOR_FLOOR);
}

float3 RTGIReconSpecularFactor(float4 albedoMetallic)
{
    float3 dielectric = float3(RTGI_RECON_DIELECTRIC_F0, RTGI_RECON_DIELECTRIC_F0, RTGI_RECON_DIELECTRIC_F0);
    return max(lerp(dielectric, albedoMetallic.rgb, saturate(albedoMetallic.a)), RTGI_RECON_FACTOR_FLOOR);
}

float RTGIReconCompressLuminance(float luminance)
{
    return luminance / (1.0 + luminance);
}

float3 RTGIReconSanitize(float3 value)
{
    if (!all(isfinite(value)))
        return 0.0;
    return clamp(value, 0.0, RTGI_RECON_STORAGE_LIMIT);
}

float RTGIReconPackHistoryLength(float length, bool hud)
{
    return hud ? -length : length;
}

float RTGIReconHistoryLength(float packedLength)
{
    return abs(packedLength);
}

bool RTGIReconHistoryHud(float packedLength)
{
    return packedLength < 0.0;
}

float RTGIReconDepthGuide(float4 surfaceData, bool valid)
{
    if (!valid || !(surfaceData.x > 0.0))
        return 0.0;
    return surfaceData.w > 0.5 ? -surfaceData.x : surfaceData.x;
}

bool RTGIReconGuideCompatible(float centerGuide, float tapGuide)
{
    return tapGuide != 0.0 && (centerGuide < 0.0) == (tapGuide < 0.0);
}

float RTGIReconSignalLuminance(float3 signal)
{
    return RTGIReconCompressLuminance(RTGIReconLuminance(signal));
}

float RTGIReconDepthGradient(float guide, float guideLeft, float guideRight, float guideUp, float guideDown)
{
    float depth = abs(guide);
    float left = RTGIReconGuideCompatible(guide, guideLeft) ? abs(abs(guideLeft) - depth) : 1e30;
    float right = RTGIReconGuideCompatible(guide, guideRight) ? abs(abs(guideRight) - depth) : 1e30;
    float up = RTGIReconGuideCompatible(guide, guideUp) ? abs(abs(guideUp) - depth) : 1e30;
    float down = RTGIReconGuideCompatible(guide, guideDown) ? abs(abs(guideDown) - depth) : 1e30;
    float horizontal = min(left, right);
    float vertical = min(up, down);
    float gradient = max(horizontal < 1e30 ? horizontal : 0.0, vertical < 1e30 ? vertical : 0.0);
    return max(gradient, depth * 1e-4);
}

#endif
