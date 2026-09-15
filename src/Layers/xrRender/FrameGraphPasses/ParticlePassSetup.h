#pragma once

#include "Layers/xrRender/FBasicVisual.h"

namespace xray::render::fg::passes {

enum class ParticleShaderVariant : u8 {
    Standard,
    Emissive,
    Distort,
    Count
};

enum ParticleBlendMode : u8 {
    PARTICLE_BLEND_SET       = 0,
    PARTICLE_BLEND_BLEND     = 1,
    PARTICLE_BLEND_ADD       = 2,
    PARTICLE_BLEND_MUL       = 3,
    PARTICLE_BLEND_MUL_2X    = 4,
    PARTICLE_BLEND_ALPHA_ADD = 5,
    PARTICLE_BLEND_COUNT     = 6,
};

struct ParticleBatch {
    fg::dxRender_Visual* visual = nullptr;
    Fmatrix worldMatrix;
    bool isHUDMode = false;
    u32 particleCount = 0;
};

} // namespace xray::render::fg::passes
