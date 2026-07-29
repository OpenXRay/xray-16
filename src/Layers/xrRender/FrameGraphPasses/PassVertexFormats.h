#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::fg::passes {

#pragma pack(push, 1)
struct SkyVertex {
    Fvector3 position;
    u32 color;
    Fvector3 texcoord0;
    Fvector3 texcoord1;
};

struct SunVertex {
    Fvector3 position;
    u32 color;
    float u, v;
};
#pragma pack(pop)

struct TextVertex {
    float x, y, z, w;
    u32 color;
    float u, v;
};

struct ParticleVertex {
    Fvector p;
    u32 color;
    Fvector2 t;
    u32 materialID;
    u32 _pad;
};
static_assert(sizeof(ParticleVertex) == 32, "ParticleVertex must be 32 bytes to match Vulkan std430 stride");

struct GPUParticleData {
    Fvector position;
    float rotation;
    Fvector2 size;
    u32 color;
    u32 materialID;
    Fvector2 uvMin;
    Fvector2 uvMax;
};
static_assert(sizeof(GPUParticleData) == 48, "GPUParticleData must be 48 bytes");

struct ScreenResCB {
    float width, height, invWidth, invHeight;
};

struct HiZCB {
    u32 outputWidth;
    u32 outputHeight;
    u32 inputMipLevel;
    u32 isFirstMip;
    u32 inputWidth;
    u32 inputHeight;
    u32 pad0;
    u32 pad1;
};

struct HistogramCB {
    float minLogLum;
    float logLumRange;
    u32 width;
    u32 height;
};

struct AdaptCB {
    float minLogLum;
    float logLumRange;
    float lowPercentile;
    float highPercentile;
    float adaptSpeedUp;
    float adaptSpeedDown;
    float deltaTime;
    float exposureCompensation;
    float minExposure;
    float maxExposure;
    float calibrationConstant;
    float padding;
};

} // namespace xray::render::fg::passes
