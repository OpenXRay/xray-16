// xrRender/FrameGraphPasses/ShaderConstants.cpp
// Implementation of sun constants helpers that require access to RImplementation

#include "stdafx.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/light.h"

namespace RENDER_NAMESPACE {
    extern CRender RImplementation;
}

namespace xray::render::RENDER_NAMESPACE::passes {

using namespace xray::render::RENDER_NAMESPACE;

void GetSunLightData(SunLightData& outSun, float hdrIntensity) {
    auto* sun = static_cast<light*>(RImplementation.Lights.sun._get());
    if (sun) {
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);
        outSun.direction = sun->direction;
        outSun.intensity = 1.f;
    }
}

} // namespace xray::render::RENDER_NAMESPACE::passes
