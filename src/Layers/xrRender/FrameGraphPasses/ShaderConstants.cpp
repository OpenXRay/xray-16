#include "stdafx.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/Light_DB.h"

namespace xray::render::fg::passes {

using namespace xray::render::fg;

void GetSunLightData(SunLightData& outSun, float hdrIntensity) {
    outSun.color.set(1.0f, 0.95f, 0.9f);
    outSun.direction.set(0.577f, -0.577f, 0.577f);
    outSun.intensity = hdrIntensity;

    auto* sun = static_cast<light*>(Lights.sun._get());
    if (sun) {
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);
        outSun.direction = sun->direction;
        outSun.intensity = hdrIntensity;
    }
}

} // namespace xray::render::fg::passes
