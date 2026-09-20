#ifndef CLUSTER_CULL_PARAMS_H
#define CLUSTER_CULL_PARAMS_H

cbuffer ClusterCullParams : register(b5)
{
    float4x4 g_HiZViewProj;
    float4 g_FrustumPlanes[6];
    float4 g_CameraPos;
    float4 g_ViewDir;
    float4 g_LodParams;
    uint g_InstanceCount;
    uint g_UseHiZ;
    uint g_HiZWidth;
    uint g_HiZHeight;
    uint g_HiZMipLevels;
    float g_SsaCull;
    float g_SwCull;
    float g_SwNearZ;
    uint g_Phase;
    uint g_SrcCountOffset;
    uint g_DstCountOffset;
    uint g_CoarseHiZ;
    uint g_RefCount;
    uint g_NodeQueueCapacity;
    uint g_LeafQueueCapacity;
    uint g_DeferredNodeCapacity;
    uint g_StaticHistoryValid;
    uint g_ResidencyStreaming;
    uint g_ParamsPad1;
    uint g_ParamsPad2;
};

#endif
