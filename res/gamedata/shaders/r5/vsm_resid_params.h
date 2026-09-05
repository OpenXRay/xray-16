#ifndef VSM_RESID_PARAMS_H
#define VSM_RESID_PARAMS_H

cbuffer VsmResidParams : register(b5)
{
    int4 g_PageBase[3];
    uint g_Frame;
    uint g_RefreshBudget;
    uint g_WrongBudget;
    uint g_ForceDirty;
    uint4 g_Interval[2];
    float4 g_Pivot;
    float4 g_Sun;
    float4 g_LevelOrigin[VSM_LEVELS];
};

#endif
