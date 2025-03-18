// FHierrarhyVisual.h: interface for the FHierrarhyVisual class.
//
//////////////////////////////////////////////////////////////////////


#pragma once

#include "FBasicVisual.h"

namespace xray::render::RENDER_NAMESPACE
{
class FHierrarhyVisual : public dxRender_Visual
{
public:
    xr_vector<dxRender_Visual*> children;
    BOOL bDontDelete;

public:
    FHierrarhyVisual();
    virtual ~FHierrarhyVisual();

    virtual void Load(const char* N, IReader* data, u32 dwFlags);
    virtual void Copy(dxRender_Visual* pFrom);
    virtual void Release();
    virtual IRenderVisual* getSubModel(u8 idx) //--#SM+#--
    {
        if (children.size() > idx)
            return children[idx];
        return NULL;
    }
};
} // namespace xray::render::RENDER_NAMESPACE
