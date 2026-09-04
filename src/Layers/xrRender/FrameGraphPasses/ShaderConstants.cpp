#include "stdafx.h"
#include "ShaderConstants.h"
#include "Layers/xrRender/light.h"
#include "Layers/xrRender/Light_DB.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Environment.h"

namespace xray::render::fg::passes {

using namespace xray::render::fg;

namespace {
Fvector s_sunDirVisual = {0.0f, -1.0f, 0.0f};
bool s_sunDirVisualInit = false;
}

const Fvector& SunDirVisual()
{
    if (g_pGamePersistent) {
        auto& env = g_pGamePersistent->Environment();
        if (!s_sunDirVisualInit || !env.IsThunderboltActive()) {
            s_sunDirVisual = env.CurrentEnv.sun_dir;
            s_sunDirVisualInit = true;
        }
    }
    return s_sunDirVisual;
}

void ResetSunDirVisual()
{
    s_sunDirVisualInit = false;
}

Fmatrix HudFovWarp()
{
    Fmatrix invView;
    invView.invert(Device.mView);
    Fmatrix scale;
    scale.identity();
    scale._11 = 1.0f / psHUD_FOV;
    scale._22 = 1.0f / psHUD_FOV;
    Fmatrix scaledView;
    scaledView.mul(scale, Device.mView);
    Fmatrix warp;
    warp.mul(invView, scaledView);
    return warp;
}

void GetSunLightData(SunLightData& outSun, float hdrIntensity) {
    auto* sun = static_cast<light*>(Lights.sun._get());
    if (sun) {
        outSun.color.set(sun->color.r, sun->color.g, sun->color.b);
        outSun.direction = sun->direction;
        outSun.intensity = 1.f;
    }
}

} // namespace xray::render::fg::passes
