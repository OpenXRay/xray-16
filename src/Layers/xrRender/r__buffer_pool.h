#pragma once

#include "Layers/xrRender/BufferUtils.h"
#include "xrCore/FMesh.hpp"
#include "xrCore/_stl_extensions.h"

namespace xray::render::fg
{
class dxRender_Visual;

typedef svector<VertexElement, MAXD3DDECLLENGTH + 1> VertexDeclarator;

struct R_buffer_pool
{
    xr_vector<VertexDeclarator> nDC, xDC;
    xr_vector<VertexStagingBuffer> nVB, xVB;
    xr_vector<IndexStagingBuffer> nIB, xIB;
    xr_vector<FSlideWindowItem> SWIs;
    xr_vector<dxRender_Visual*> Visuals;

    bool fastGeomLoaded = false;

    dxRender_Visual* getVisual(int id)
    {
        VERIFY(id < int(Visuals.size()));
        return Visuals[id];
    }

    VertexElement* getVB_Format(int id, bool alternative = false)
    {
        if (alternative)
        {
            VERIFY(id < int(xDC.size()));
            return xDC[id].begin();
        }
        VERIFY(id < int(nDC.size()));
        return nDC[id].begin();
    }

    VertexStagingBuffer* getVB(int id, bool alternative = false)
    {
        if (alternative)
        {
            VERIFY(id < int(xVB.size()));
            return &xVB[id];
        }
        VERIFY(id < int(nVB.size()));
        return &nVB[id];
    }

    IndexStagingBuffer* getIB(int id, bool alternative = false)
    {
        if (alternative)
        {
            VERIFY(id < int(xIB.size()));
            return &xIB[id];
        }
        VERIFY(id < int(nIB.size()));
        return &nIB[id];
    }

    FSlideWindowItem* getSWI(int id)
    {
        VERIFY(id < int(SWIs.size()));
        return &SWIs[id];
    }
};

extern ECORE_API R_buffer_pool BufferPool;

class CModelPool;
extern ECORE_API CModelPool* g_pModelPool;

}
