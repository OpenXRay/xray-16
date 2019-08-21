#ifndef dxFontRender_included
#define dxFontRender_included
#pragma once

#include "Include/xrRender/FontRender.h"

class dxFontRender : public IFontRender
{
public:
    dxFontRender() = default;
    ~dxFontRender() override;

    void Initialize(cpcstr cShader, cpcstr cTexture) override;
    void OnRender(CGameFont& owner) override;

private:
    ref_shader pShader;
    ref_geom pGeom;
};

#endif //	FontRender_included
