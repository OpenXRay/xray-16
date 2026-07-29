#include "stdafx.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/Light_DB.h"

namespace xray::render::fg::passes {

using namespace xray::render::fg;

void GetSunLightData(SunLightData& outSun, float hdrIntensity) {
    outSun.color.set(0.f, 0.f, 0.f);
    outSun.direction.set(0.f, -1.f, 0.f);
    outSun.intensity = 0.f;

    auto* sun = static_cast<light*>(Lights.sun._get());
    if (sun) {
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);
        outSun.direction = sun->direction;
        outSun.intensity = (hdrIntensity > 0.f) ? hdrIntensity : 1.f;
    }
}

} // namespace xray::render::fg::passes
