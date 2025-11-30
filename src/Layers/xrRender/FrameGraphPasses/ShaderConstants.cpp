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

        outSun.intensity = hdrIntensity;
    } else {
        outSun.color.set(1.0f, 0.95f, 0.9f);
        outSun.direction.set(0.577f, -0.577f, 0.577f);
        outSun.intensity = hdrIntensity;
    }
}

}
