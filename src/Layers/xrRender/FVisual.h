// FVisual.h: interface for the FVisual class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "FBasicVisual.h"

namespace xray::render::fg
{
class Fvisual : public dxRender_Visual, public IRender_Mesh
{
public:
    IRender_Mesh* m_fast;

public:
    virtual void Load(LPCSTR N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();

    Fvisual();
    virtual ~Fvisual();
};
} // namespace xray::render::fg
