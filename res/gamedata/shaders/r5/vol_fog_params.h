#ifndef VOL_FOG_PARAMS_H
#define VOL_FOG_PARAMS_H

cbuffer VolFogParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4x4 g_PrevViewProj;
    float4 g_CameraPos;
    float4 g_SunDir;
    float4 g_SunColor;
    float4 g_FogTune;
    float4 g_FogTune2;
    float4 g_FogColor;
    float2 g_ScreenSize;
    float g_ZNear;
    float g_ZFar;
    uint g_FrameIndex;
    uint g_EnableGI;
    uint g_EnableSun;
    uint g_EnableRT;
    uint g_EnableLights;
    uint g_EnableTemporal;
    uint g_SpotMode;
    uint g_NumLights;
    float g_AtmosphereStrength;
    uint g_EnableAtmosphere;
    uint g_PlayerLight;
    uint g_PadFog;
    float4 g_SkyColor;
    float4 g_HemiColor;
};

#endif
