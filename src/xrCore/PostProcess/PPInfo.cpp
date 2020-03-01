#include "stdafx.h"
#include "PPInfo.hpp"

SPPInfo& SPPInfo::add(const SPPInfo& ppi)
{
    blur += ppi.blur;
    gray += ppi.gray;
    duality.h += ppi.duality.h;
    duality.v += ppi.duality.v;

    noise.intensity = _max(noise.intensity, ppi.noise.intensity);
    noise.grain = _max(noise.grain, ppi.noise.grain);
    noise.fps = _max(noise.fps, ppi.noise.fps);
    color_base += ppi.color_base;
    color_gray += ppi.color_gray;
    color_add += ppi.color_add;

    if (ppi.cm_tex1.size())
    {
        if (cm_tex1.size())
        {
            cm_tex2 = ppi.cm_tex1;
            cm_interpolate = 1.0f - cm_influence / (cm_influence + ppi.cm_influence);
        }
        else
        {
            cm_tex1 = ppi.cm_tex1;
            cm_influence = ppi.cm_influence;
            cm_interpolate = 0.0f;
        }
        cm_influence = _max(cm_influence, ppi.cm_influence);
    }
    return *this;
}

SPPInfo& SPPInfo::sub(const SPPInfo& ppi)
{
    blur -= ppi.blur;
    gray -= ppi.gray;
    duality.h -= ppi.duality.h;
    duality.v -= ppi.duality.v;
    color_base -= ppi.color_base;
    color_gray -= ppi.color_gray;
    color_add -= ppi.color_add;
    return *this;
}

SPPInfo::SPPInfo()
{
    blur = gray = duality.h = duality.v = 0;
    noise.intensity = 0;
    noise.grain = 1;
    noise.fps = 10;
    color_base.set(.5f, .5f, .5f);
    color_gray.set(.333f, .333f, .333f);
    color_add.set(0.f, 0.f, 0.f);
    cm_influence = 0.0f;
    cm_interpolate = 0.0f;
}
void SPPInfo::normalize() {}
void SPPInfo::validate(LPCSTR str)
{
    VERIFY2(_valid(duality.h), str);
    VERIFY2(_valid(duality.v), str);
    VERIFY2(_valid(blur), str);
    VERIFY2(_valid(gray), str);
    VERIFY2(_valid(noise.intensity), str);
    VERIFY2(_valid(noise.grain), str);
    VERIFY2(_valid(noise.fps), str);
    VERIFY2(_valid(color_base.r), str);
    VERIFY2(_valid(color_base.g), str);
    VERIFY2(_valid(color_base.b), str);
    VERIFY2(_valid(color_gray.r), str);
    VERIFY2(_valid(color_gray.g), str);
    VERIFY2(_valid(color_gray.b), str);
    VERIFY2(_valid(color_add.r), str);
    VERIFY2(_valid(color_add.g), str);
    VERIFY2(_valid(color_add.b), str);
}

SPPInfo& SPPInfo::lerp(const SPPInfo& def, const SPPInfo& to, float factor)
{
    VERIFY(_valid(factor));
    SPPInfo& pp = *this;
    clamp(factor, 0.0f, 1.0f);

    pp.duality.h += def.duality.h + (to.duality.h - def.duality.h) * factor;
    pp.duality.v += def.duality.v + (to.duality.v - def.duality.v) * factor;
    pp.gray += def.gray + (to.gray - def.gray) * factor;
    pp.blur += def.blur + (to.blur - def.blur) * factor;
    pp.noise.intensity = to.noise.intensity; // + (to.noise.intensity - def.noise.intensity) * factor;
    pp.noise.grain = to.noise.grain; // + (to.noise.grain - def.noise.grain) * factor;
    pp.noise.fps = to.noise.fps; // + (to.noise.fps - def.noise.fps) * factor;

    pp.color_base.set(def.color_base.r + (to.color_base.r - def.color_base.r) * factor,
        def.color_base.g + (to.color_base.g - def.color_base.g) * factor,
        def.color_base.b + (to.color_base.b - def.color_base.b) * factor);

    pp.color_gray.set(def.color_gray.r + (to.color_gray.r - def.color_gray.r) * factor,
        def.color_gray.g + (to.color_gray.g - def.color_gray.g) * factor,
        def.color_gray.b + (to.color_gray.b - def.color_gray.b) * factor);

    pp.color_add.set(def.color_add.r + (to.color_add.r - def.color_add.r) * factor,
        def.color_add.g + (to.color_add.g - def.color_add.g) * factor,
        def.color_add.b + (to.color_add.b - def.color_add.b) * factor);

    pp.cm_tex1 = to.cm_tex1;
    pp.cm_tex2 = to.cm_tex2;
    pp.cm_influence += def.cm_influence + (to.cm_influence - def.cm_influence) * factor;
    pp.cm_interpolate += def.cm_interpolate + (to.cm_interpolate - def.cm_interpolate) * factor;

    return *this;
}
