// xrRender/FrameGraphPasses/ShaderConstants.cpp
// Implementation of sun constants helpers that require access to RImplementation

#include "stdafx.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/light.h"

namespace fg {
    extern CRender RImplementation;
}

namespace xray::render::fg::passes {

using namespace xray::render::fg;

void GetSunLightData(SunLightData& outSun, float hdrIntensity) {
    auto* sun = static_cast<light*>(RImplementation.Lights.sun._get());
    if (sun) {
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);
        outSun.direction = sun->direction;
        outSun.intensity = 1.f;
    }
}

} // namespace xray::render::fg::passes
