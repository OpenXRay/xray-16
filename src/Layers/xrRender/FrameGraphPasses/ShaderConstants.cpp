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

static Fmatrix HudFovViewScale(const Fmatrix& view, float scaleXY)
{
    Fmatrix invView;
    invView.invert(view);
    Fmatrix scale;
    scale.identity();
    scale._11 = scaleXY;
    scale._22 = scaleXY;
    Fmatrix scaledView;
    scaledView.mul(scale, view);
    Fmatrix warp;
    warp.mul(invView, scaledView);
    return warp;
}

Fmatrix HudFovWarp(const Fmatrix& view)
{
    return HudFovViewScale(view, 1.0f / psHUD_FOV);
}

Fmatrix HudFovUnwarp(const Fmatrix& view)
{
    return HudFovViewScale(view, psHUD_FOV);
}

Fmatrix HudFovWarp()
{
    return HudFovWarp(Device.mView);
}

Fmatrix HudFovUnwarp()
{
    return HudFovUnwarp(Device.mView);
}

void MergeBoundingSphere(Fvector4& acc, const Fvector4& b)
{
    Fvector c0;
    c0.set(acc.x, acc.y, acc.z);
    Fvector c1;
    c1.set(b.x, b.y, b.z);
    Fvector d;
    d.sub(c1, c0);
    const float dist = d.magnitude();
    if (dist + b.w <= acc.w)
        return;
    if (dist + acc.w <= b.w) {
        acc = b;
        return;
    }
    const float r = 0.5f * (dist + acc.w + b.w);
    c0.mad(d, (r - acc.w) / dist);
    acc.set(c0.x, c0.y, c0.z, r);
}

Fvector4 HudShadowSphere(const Fvector4& sphere)
{
    Fvector4 fit = sphere;
    fit.w = std::max(sphere.w, 0.05f) + kHudBoundsMargin;
    return fit;
}

void GetSunLightData(SunLightData& outSun) {
    auto* sun = static_cast<light*>(Lights.sun._get());
    if (sun) {
        outSun.color = SrgbToLinear(Fvector().set(sun->color.r, sun->color.g, sun->color.b));
        outSun.direction = sun->direction;
    }
}

} // namespace xray::render::fg::passes
