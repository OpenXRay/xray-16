#ifndef RTGI_RAW_PARAMS_H
#define RTGI_RAW_PARAMS_H

cbuffer RTGIRawParams : register(b5)
{
    float4x4 g_InvViewProj;
    float4 g_CameraPos;
    float4 g_SunDir_Intensity;
    float4 g_SunColor;
    float g_ScreenWidth, g_ScreenHeight, g_GIIntensity; uint g_FrameIndex;
    uint g_IdentityStaticCount, g_TerrainBatchCount, g_SkinnedBatchStart, g_GrassBatchStart;
    uint g_DetailAtlasIndex, g_DiffuseMode, g_RTLightCount, g_EmissiveCount;
    uint g_MaxNullEvents, g_MaxBounces, g_SamplesPerPixel; float g_RayDistance;
    uint g_RawPad3; float g_SunAngularRadius, g_CameraConeSpread; uint g_ClusterLights;
    uint g_DetailMeshBatchStart, g_StaticDetailBatchStart, g_DetailPbrIndex, g_DetailBumpIndex;
    uint g_LightRays, g_RawPad0, g_RawPad1, g_RawPad2;
};

#endif
