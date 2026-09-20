#ifndef CLUSTER_VIS_PS_H
#define CLUSTER_VIS_PS_H

#include "visbuffer_common.h"
#include "cluster_fade.h"
#ifdef CLUSTER_VIS_AT
#include "bindless_common.h"
#endif


struct PS_INPUT
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD0;
    nointerpolation uint materialID : TEXCOORD1;
    nointerpolation uint drawID : TEXCOORD2;
#if !defined(CLUSTER_VIS_MESH) || !defined(TARGET_SPIRV)
    nointerpolation uint visID : TEXCOORD3;
#endif
};

#if defined(CLUSTER_VIS_MESH) && defined(TARGET_SPIRV)
uint ClusterVisID(PS_INPUT input)
{
    return asuint(spirv_asm
    {
        OpCapability MeshShadingEXT;
        OpExtension "SPV_EXT_mesh_shader";
        OpDecorate builtin(PrimitiveId:int) Flat;
        result:$$int = OpLoad builtin(PrimitiveId:int);
    });
}
#else
uint ClusterVisID(PS_INPUT input)
{
    return input.visID;
}
#endif

uint main(PS_INPUT input) : SV_Target0
{
    ClusterFadeDiscard(g_DrawFades[input.drawID], input.position);

#ifdef CLUSTER_VIS_AT
    MaterialData mat = g_Materials[input.materialID];
    if (mat.flags & MAT_FLAG_ALPHA_TEST)
    {
        float4 diffuseSample = SampleDiffuse(mat, input.texcoord);
        clip(diffuseSample.a - mat.alphaRef);
    }
#endif

    return ClusterVisID(input);
}

#endif
