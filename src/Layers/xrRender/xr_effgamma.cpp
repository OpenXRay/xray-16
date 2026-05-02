#include "stdafx.h"
#include "xr_effgamma.h"

namespace xray::render::fg
{
IC u16 clr2gamma(float c)
{
    int C = iFloor(c);
    clamp(C, 0, 65535);
    return u16(C);
}

void CGammaControl::GenLUT(u16* r, u16* g, u16* b, u16 count) const
{
    const float og = 1.f / (fGamma + EPS);
    const float B = fBrightness / 2.f;
    const float C = fContrast / 2.f;
    for (u16 i = 0; i < count; i++)
    {
        const float c = (C + .5f) * powf(float(i) / 255.f, og) * 65535.f + (B - 0.5f) * 32768.f - C * 32768.f + 16384.f;
        r[i] = clr2gamma(c * cBalance.r);
        g[i] = clr2gamma(c * cBalance.g);
        b[i] = clr2gamma(c * cBalance.b);
    }
}

void CGammaControl::Update() const
{
    (void)Device;
}
}
