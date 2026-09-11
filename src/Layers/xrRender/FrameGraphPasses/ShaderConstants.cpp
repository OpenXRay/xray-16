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

HudShadowFit BuildHudShadowFit(const Fvector4& trueSphere)
{
    HudShadowFit fit;
    fit.warp = HudFovWarp();
    fit.trueSphere = trueSphere;
    fit.trueSphere.w = std::max(trueSphere.w, 0.05f) + kHudBoundsMargin;
    Fvector center;
    center.set(trueSphere.x, trueSphere.y, trueSphere.z);
    fit.warp.transform_tiny(center);
    const float stretch = std::max(1.0f / psHUD_FOV, 1.0f);
    fit.shownSphere.set(center.x, center.y, center.z, std::max(trueSphere.w, 0.05f) * stretch + kHudBoundsMargin);
    return fit;
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
