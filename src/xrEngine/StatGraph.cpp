#include "stdafx.h"
#pragma hdrstop

#include "StatGraph.h"
//---------------------------------------------

CStatGraph::CStatGraph()
{
    Device.seqRender.Add(this, REG_PRIORITY_LOW - 1000);
    OnDeviceCreate();
    mn = 0;
    mx = 1;
    max_item_count = 1;
    lt.set(0, 0);
    rb.set(200, 200);
    grid.set(1, 1);
    grid_color = 0xFF000000;
    rect_color = 0xFF000000;
    grid_step.set(1, 1);

    AppendSubGraph(stCurve);
}

CStatGraph::~CStatGraph()
{
    Device.seqRender.Remove(this);
    OnDeviceDestroy();
    m_Markers.clear();
}

void CStatGraph::OnDeviceCreate()
{
    m_pRender->OnDeviceCreate();
}

void CStatGraph::OnDeviceDestroy()
{
    m_pRender->OnDeviceDestroy();
}

void CStatGraph::OnRender()
{
    m_pRender->OnRender(*this);
};
