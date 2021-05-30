#pragma once
#ifndef dxStatGraphRender_included
#define dxStatGraphRender_included

#include "Include/xrRender/StatGraphRender.h"
#include "xrEngine/StatGraph.h"

class dxStatGraphRender : public IStatGraphRender
{
public:
    virtual void Copy(IStatGraphRender& _in);

    virtual void OnDeviceCreate();
    virtual void OnDeviceDestroy();
    virtual void OnRender(CStatGraph& owner);

private:
    void RenderBack(CStatGraph& owner);
    static void RenderBars(CStatGraph& owner, FVF::L** ppv, CStatGraph::ElementsDeq& pelements);
    void RenderBarLines(CStatGraph& owner, FVF::L** ppv, CStatGraph::ElementsDeq& pelements) const;
    void RenderLines(CStatGraph& owner, FVF::L** ppv, CStatGraph::ElementsDeq& pelements) const;
    static void RenderMarkers(CStatGraph& owner, FVF::L** ppv, CStatGraph::MarkersDeq& pmarkers);

private:
    ref_geom hGeomTri;
    ref_geom hGeomLine;
};

#endif //	dxStatGraphRender_included
