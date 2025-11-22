#ifndef OPENXRAY_UBO_RENDER_DUMP_H
#define OPENXRAY_UBO_RENDER_DUMP_H

layout(std140) uniform RenderDumpUBO {
    float4x4 xform;
    float4x4 formView;
    float4 consts;
    float4 scale;
    float4 bias;
    float4 wind;
    float4 wave;
    float4 sun;
};

#endif