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
    // Access the sun light from RImplementation
    // This is the "fuckingsun" pattern from vanilla X-Ray
    auto* sun = static_cast<light*>(RImplementation.Lights.sun._get());

    if (sun) {
        // Get sun color from light
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);

        // Get sun direction (world space)
        // X-Ray stores direction FROM the sun (toward scene)
        outSun.direction = sun->direction;

        // HDR intensity multiplier
        outSun.intensity = 1.f;

        Msg("* [Sun] color=(%.3f, %.3f, %.3f), dir=(%.3f, %.3f, %.3f)",
            sun->color.r, sun->color.g, sun->color.b,
            sun->direction.x, sun->direction.y, sun->direction.z);
    } else {
        // Fallback if no sun available
        outSun.color.set(1.0f, 0.95f, 0.9f);  // Warm white
        outSun.direction.set(0.577f, -0.577f, 0.577f);  // Diagonal down
        outSun.intensity = hdrIntensity;

        Msg("! [Sun] Not available, using fallback");
    }
}

} // namespace xray::render::RENDER_NAMESPACE::passes
