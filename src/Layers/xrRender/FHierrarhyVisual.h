// FHierrarhyVisual.h: interface for the FHierrarhyVisual class.
//
//////////////////////////////////////////////////////////////////////

#ifndef FHierrarhyVisualH
#define FHierrarhyVisualH

#pragma once

#include "FBasicVisual.h"

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

#endif // FHierrarhyVisualH
