// FVisual.h: interface for the FVisual class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "FBasicVisual.h"

namespace xray::render::RENDER_NAMESPACE
{
class Fvisual : public dxRender_Visual, public IRender_Mesh
{
public:
    IRender_Mesh* m_fast;

public:
    virtual void Render(CBackend& cmd_list, float LOD, bool use_fast_geo) override; // LOD - Level Of Detail  [0.0f - min, 1.0f - max], Ignored ?
    virtual void Load(LPCSTR N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

    Fvisual();
    virtual ~Fvisual();
};
} // namespace xray::render::RENDER_NAMESPACE
