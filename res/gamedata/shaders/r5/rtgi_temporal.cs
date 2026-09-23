#include "rtgi_reconstruct_common.h"

cbuffer RTGITemporalParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4x4 g_PrevInvViewProj;
    float4x4 g_View;
    float4x4 g_PrevView;
    float4x4 g_InvProj;
    float4x4 g_PrevInvProj;
    float4 g_CameraPos;
    float g_ScreenWidth, g_ScreenHeight, g_InvScreenWidth, g_InvScreenHeight;
    uint g_HistoryValid, g_MaxHistory; float g_PlaneTolerance, g_NormalTolerance;
    uint g_FrameIndex; float g_HudFov; uint g_TemporalPad1, g_TemporalPad2;
};

Texture2D<float4> t_RawDiffuse : register(t0);
Texture2D<float4> t_RawSpecular : register(t1);
Texture2D<float4> t_NormalRoughness : register(t2);
Texture2D<float4> t_AlbedoMetallic : register(t3);
Texture2D<float4> t_SurfaceData : register(t4);
Texture2D<float2> t_Motion : register(t5);
Texture2D<float> t_PrevDepth : register(t6);
Texture2D<float4> t_PrevNormal : register(t7);
Texture2D<float4> t_PrevHistoryDiffuse : register(t8);
Texture2D<float4> t_PrevHistorySpecular : register(t9);
Texture2D<float4> t_PrevMoments : register(t10);
Texture2D<float2> t_PrevFast : register(t11);

RWTexture2D<float4> u_HistoryDiffuse : register(u0);
RWTexture2D<float4> u_HistorySpecular : register(u1);
RWTexture2D<float4> u_Moments : register(u2);
RWTexture2D<float2> u_Fast : register(u3);
RWTexture2D<float4> u_Reconstruction : register(u4);

struct RTGITemporalHistory
{
    float3 diffuse;
    float3 specular;
    float4 moments;
    float2 fast;
    float diffuseLength;
    float specularLength;
    float weight;
    uint reason;
};

float3 RTGITemporalWorldPos(float2 pixelCenter, float depth, float4x4 invViewProj)
{
    float2 uv = pixelCenter * float2(g_InvScreenWidth, g_InvScreenHeight);
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, depth, 1.0);
    float4 world = mul(invViewProj, clip);
    return world.xyz / world.w;
}

float3 RTGITemporalHudViewPos(float2 pixelCenter, float depth, float4x4 invProj)
{
    float2 uv = pixelCenter * float2(g_InvScreenWidth, g_InvScreenHeight);
    float4 clip = float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, saturate((depth - RTGI_RECON_HUD_DEPTH) * 10.0), 1.0);
    float4 view = mul(invProj, clip);
    float3 position = view.xyz / view.w;
    position.xy *= g_HudFov;
    return position;
}

float3 RTGITemporalViewNormal(float3 N, float4x4 view)
{
    return normalize(mul((float3x3)view, N));
}

float3 RTGITemporalPosition(float2 pixelCenter, float depth, bool hud, bool previous)
{
    if (hud)
        return RTGITemporalHudViewPos(pixelCenter, depth, previous ? g_PrevInvProj : g_InvProj);
    return RTGITemporalWorldPos(pixelCenter, depth, previous ? g_PrevInvViewProj : g_InvViewProj);
}

float3 RTGITemporalNormal(float3 N, bool hud, bool previous)
{
    if (hud)
        return RTGITemporalViewNormal(N, previous ? g_PrevView : g_View);
    return N;
}

void RTGITemporalTap(inout RTGITemporalHistory history, int2 tap, float weight, float3 position, float3 N,
    float planeTolerance, bool hud)
{
    if (!(weight > 0.0) || tap.x < 0 || tap.y < 0 || tap.x >= int(g_ScreenWidth) || tap.y >= int(g_ScreenHeight))
        return;
    float prevDepth = t_PrevDepth.Load(int3(tap, 0));
    float4 prevNormalData = t_PrevNormal.Load(int3(tap, 0));
    float4 prevDiffuse = t_PrevHistoryDiffuse.Load(int3(tap, 0));
    bool prevHud = prevDepth >= RTGI_RECON_HUD_DEPTH;
    if (!isfinite(prevDepth) || prevDepth <= 0.0 || prevHud != hud ||
        !all(isfinite(prevNormalData.xyz)) || dot(prevNormalData.xyz, prevNormalData.xyz) < 0.25 ||
        !all(isfinite(prevDiffuse)) || !(prevDiffuse.a > 0.0))
    {
        history.reason = RTGI_RECON_REJECT_GEOMETRY;
        return;
    }
    float3 prevPosition = RTGITemporalPosition(float2(tap) + 0.5, prevDepth, hud, true);
    if (!all(isfinite(prevPosition)) || abs(dot(prevPosition - position, N)) > planeTolerance)
    {
        history.reason = RTGI_RECON_REJECT_GEOMETRY;
        return;
    }
    float3 prevN = RTGITemporalNormal(normalize(prevNormalData.xyz), hud, true);
    if (dot(prevN, N) < g_NormalTolerance)
    {
        history.reason = RTGI_RECON_REJECT_NORMAL;
        return;
    }
    float4 prevSpecular = t_PrevHistorySpecular.Load(int3(tap, 0));
    float4 prevMoments = t_PrevMoments.Load(int3(tap, 0));
    float2 prevFast = t_PrevFast.Load(int3(tap, 0));
    if (!all(isfinite(prevSpecular)) || !all(isfinite(prevMoments)) || !all(isfinite(prevFast)))
    {
        history.reason = RTGI_RECON_REJECT_GEOMETRY;
        return;
    }
    history.diffuse += prevDiffuse.rgb * weight;
    history.specular += prevSpecular.rgb * weight;
    history.moments += prevMoments * weight;
    history.fast += prevFast * weight;
    history.diffuseLength += prevDiffuse.a * weight;
    history.specularLength += prevSpecular.a * weight;
    history.weight += weight;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchID.xy;
    if (pixel.x >= uint(g_ScreenWidth) || pixel.y >= uint(g_ScreenHeight))
        return;

    float4 rawDiffuse = t_RawDiffuse.Load(int3(pixel, 0));
    float4 rawSpecular = t_RawSpecular.Load(int3(pixel, 0));
    float4 normalRoughness = t_NormalRoughness.Load(int3(pixel, 0));
    float4 albedoMetallic = t_AlbedoMetallic.Load(int3(pixel, 0));
    float4 surfaceData = t_SurfaceData.Load(int3(pixel, 0));
    float2 motion = t_Motion.Load(int3(pixel, 0));
    float maxHistory = float(max(g_MaxHistory, 1u));

    bool primaryValid = rawDiffuse.a > 0.5 && all(isfinite(rawDiffuse)) && all(isfinite(rawSpecular)) &&
        all(isfinite(normalRoughness)) && all(isfinite(albedoMetallic)) && all(isfinite(surfaceData)) &&
        dot(normalRoughness.xyz, normalRoughness.xyz) >= 0.25 && surfaceData.y > 0.0;
    if (!primaryValid)
    {
        u_HistoryDiffuse[pixel] = 0.0;
        u_HistorySpecular[pixel] = 0.0;
        u_Moments[pixel] = 0.0;
        u_Fast[pixel] = 0.0;
        u_Reconstruction[pixel] = float4(0.0, 0.0, float(RTGI_RECON_REJECT_PRIMARY) * RTGI_RECON_REJECT_SCALE, 0.0);
        return;
    }

    float3 N = normalize(normalRoughness.xyz);
    float roughness = saturate(normalRoughness.w);
    float3 diffuse = RTGIReconSanitize(rawDiffuse.rgb / RTGIReconDiffuseFactor(albedoMetallic));
    float3 specular = RTGIReconSanitize(rawSpecular.rgb / RTGIReconSpecularFactor(albedoMetallic));
    float diffuseLum = RTGIReconCompressLuminance(RTGIReconLuminance(diffuse));
    float specularLum = RTGIReconCompressLuminance(RTGIReconLuminance(specular));
    float4 currentMoments = float4(diffuseLum, diffuseLum * diffuseLum, specularLum, specularLum * specularLum);
    float2 currentFast = float2(diffuseLum, specularLum);

    RTGITemporalHistory history = (RTGITemporalHistory)0;
    bool hud = surfaceData.w > 0.5;
    bool motionValid = surfaceData.z > 2.5 && all(isfinite(motion));
    if (g_HistoryValid == 0u)
        history.reason = RTGI_RECON_REJECT_NO_HISTORY;
    else if (!motionValid)
        history.reason = RTGI_RECON_REJECT_MOTION;
    else
    {
        float3 position = RTGITemporalPosition(float2(pixel) + 0.5, surfaceData.y, hud, false);
        float3 referenceN = RTGITemporalNormal(N, hud, false);
        float distance = hud ? length(position) : surfaceData.x;
        float planeTolerance = max(g_PlaneTolerance * distance, hud ? 0.002 : 0.01);
        float2 currUV = (float2(pixel) + 0.5) * float2(g_InvScreenWidth, g_InvScreenHeight);
        float2 prevUV = currUV + motion;
        if (!all(isfinite(position)) || any(prevUV < 0.0) || any(prevUV >= 1.0))
            history.reason = RTGI_RECON_REJECT_OFFSCREEN;
        else
        {
            history.reason = RTGI_RECON_REJECT_GEOMETRY;
            float2 prevPixel = prevUV * float2(g_ScreenWidth, g_ScreenHeight) - 0.5;
            int2 base = int2(floor(prevPixel));
            float2 f = prevPixel - float2(base);
            RTGITemporalTap(history, base, (1.0 - f.x) * (1.0 - f.y), position, referenceN, planeTolerance, hud);
            RTGITemporalTap(history, base + int2(1, 0), f.x * (1.0 - f.y), position, referenceN, planeTolerance, hud);
            RTGITemporalTap(history, base + int2(0, 1), (1.0 - f.x) * f.y, position, referenceN, planeTolerance, hud);
            RTGITemporalTap(history, base + int2(1, 1), f.x * f.y, position, referenceN, planeTolerance, hud);
            if (!(history.weight > 1e-3))
            {
                uint reason = history.reason;
                history = (RTGITemporalHistory)0;
                history.reason = reason;
                int2 nearest = int2(round(prevPixel));
                for (int y = -1; y <= 1; ++y)
                    for (int x = -1; x <= 1; ++x)
                        RTGITemporalTap(history, nearest + int2(x, y), 1.0, position, referenceN, planeTolerance, hud);
            }
        }
    }

    float diffuseLength = 1.0;
    float specularLength = 1.0;
    float4 moments = currentMoments;
    float2 fast = currentFast;
    if (history.weight > 1e-3)
    {
        float invWeight = 1.0 / history.weight;
        float3 prevDiffuse = history.diffuse * invWeight;
        float3 prevSpecular = history.specular * invWeight;
        float4 prevMoments = history.moments * invWeight;
        float2 prevFast = history.fast * invWeight;
        fast = lerp(prevFast, currentFast, RTGI_RECON_FAST_ALPHA);

        float motionPixels = length(motion * float2(g_ScreenWidth, g_ScreenHeight));
        float glossy = 1.0 - saturate(roughness * 2.0);
        float specularMax = max(2.0, maxHistory * (1.0 - glossy * saturate(motionPixels)));
        diffuseLength = min(history.diffuseLength * invWeight + 1.0, maxHistory);
        specularLength = min(history.specularLength * invWeight + 1.0, specularMax);

        float diffuseSigma = sqrt(max(prevMoments.y - prevMoments.x * prevMoments.x, 0.0));
        float specularSigma = sqrt(max(prevMoments.w - prevMoments.z * prevMoments.z, 0.0));
        if (abs(fast.x - prevMoments.x) > RTGI_RECON_CHANGE_SIGMA * diffuseSigma + RTGI_RECON_CHANGE_EPSILON)
            diffuseLength = min(diffuseLength, RTGI_RECON_CHANGE_HISTORY);
        if (abs(fast.y - prevMoments.z) > RTGI_RECON_CHANGE_SIGMA * specularSigma + RTGI_RECON_CHANGE_EPSILON)
            specularLength = min(specularLength, RTGI_RECON_CHANGE_HISTORY);

        float alphaDiffuse = 1.0 / diffuseLength;
        float alphaSpecular = 1.0 / specularLength;
        diffuse = lerp(prevDiffuse, diffuse, alphaDiffuse);
        specular = lerp(prevSpecular, specular, alphaSpecular);
        moments.xy = lerp(prevMoments.xy, currentMoments.xy, alphaDiffuse);
        moments.zw = lerp(prevMoments.zw, currentMoments.zw, alphaSpecular);
        history.reason = RTGI_RECON_REJECT_NONE;
    }

    diffuse = RTGIReconSanitize(diffuse);
    specular = RTGIReconSanitize(specular);
    if (!all(isfinite(moments)))
        moments = currentMoments;
    if (!all(isfinite(fast)))
        fast = currentFast;
    float diffuseVariance = max(moments.y - moments.x * moments.x, 0.0);

    u_HistoryDiffuse[pixel] = float4(diffuse, diffuseLength);
    u_HistorySpecular[pixel] = float4(specular, specularLength);
    u_Moments[pixel] = moments;
    u_Fast[pixel] = fast;
    u_Reconstruction[pixel] = float4(diffuseLength / maxHistory, specularLength / maxHistory,
        float(history.reason) * RTGI_RECON_REJECT_SCALE, diffuseVariance);
}
