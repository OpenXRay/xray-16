#ifndef VSM_PARAMS_H
#define VSM_PARAMS_H

cbuffer VsmParams : register(b6)
{
    float4x4 vsm_view;
    float4 vsm_level[VSM_LEVELS];
    float4 vsm_zparams;
};

#endif
