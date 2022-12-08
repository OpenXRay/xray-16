//---------------------------------------------------------------------------
#ifndef StatGraphH
#define StatGraphH

#include "Include/xrRender/FactoryPtr.h"
#include "Include/xrRender/StatGraphRender.h"
#include "xrCommon/xr_deque.h"

//---------------------------------------------------------------------------
class ENGINE_API CStatGraph : public pureRender
{
    friend class dxStatGraphRender;

public:
    enum EStyle
    {
        stBar,
        stCurve,
        stBarLine,
        stPoint,
        stVert,
        stHor,
    };

protected:
    struct SElement
    {
        u32 color;
        float data;
        SElement(float d, u32 clr)
        {
            color = clr;
            data = d;
        }
    };
    using ElementsDeq = xr_deque<SElement>;

    struct SSubGraph
    {
        EStyle style;
        ElementsDeq elements;
        SSubGraph(EStyle s) { style = s; };
        void SetStyle(EStyle s) { style = s; };
    };
    using SubGraphVec = xr_vector<SSubGraph>;
    SubGraphVec subgraphs;

    float mn, mx;
    u32 max_item_count;
    Fvector2 lt, rb;
    Ivector2 grid;
    Fvector2 grid_step;
    u32 grid_color;
    u32 base_color;
    u32 rect_color;
    u32 back_color;
    FactoryPtr<IStatGraphRender> m_pRender;
    // ref_geom hGeomTri;
    // ref_geom hGeomLine;

    struct SMarker
    {
        EStyle m_eStyle;
        float m_fPos;
        u32 m_dwColor;
    };
    using MarkersDeq = xr_deque<SMarker>;
    MarkersDeq m_Markers;

protected:
    // virtual void RenderBack ();

    // virtual void RenderBars ( FVF::TL0uv** ppv, ElementsDeq* pelements );
    // virtual void RenderLines ( FVF::TL0uv** ppv, ElementsDeq* pelements );
    // virtual void RenderBarLines ( FVF::TL0uv** ppv, ElementsDeq* pelements );
    //// virtual void RenderPoints ( FVF::TL0uv** ppv, ElementsDeq* pelements );
    // virtual void RenderMarkers ( FVF::TL0uv** ppv, MarkersDeq* pmarkers );
public:
    explicit CStatGraph(bool bRegister = true);
    ~CStatGraph();
    virtual void OnRender();
    void OnDeviceCreate();
    void OnDeviceDestroy();

    void SetStyle(EStyle s, u32 SubGraphID = 0)
    {
        if (SubGraphID >= subgraphs.size())
            return;
        auto it = subgraphs.begin() + SubGraphID;
        it->SetStyle(s);
    }

    void SetRect(int l, int t, int w, int h, u32 rect_clr, u32 back_clr)
    {
        lt.set(l, t);
        rb.set(l + w, t + h);
        rect_color = rect_clr;
        back_color = back_clr;
    }

    void SetGrid(int w_div, float w_step, int h_div, float h_step, u32 grid_clr, u32 base_clr)
    {
        grid.set(w_div, h_div);
        grid_step.set(w_step, h_step);
        grid_color = grid_clr;
        base_color = base_clr;
    }

    void SetMinMax(float _mn, float _mx, u32 item_count)
    {
        mn = _mn;
        mx = _mx;
        max_item_count = item_count;
        for (auto it = subgraphs.begin(); it != subgraphs.end(); ++it)
        {
            while (it->elements.size() > max_item_count)
                it->elements.pop_front();
        };
    }

    void AppendItem(float d, u32 clr, u32 SubGraphID = 0)
    {
        if (SubGraphID >= subgraphs.size())
            return;

        clamp(d, mn, mx);

        auto it = subgraphs.begin() + SubGraphID;
        it->elements.push_back(SElement(d, clr));
        while (it->elements.size() > max_item_count)
            it->elements.pop_front();
    };

    u32 AppendSubGraph(EStyle S)
    {
        subgraphs.push_back(SSubGraph(S));
        return subgraphs.size() - 1;
    };

    void AddMarker(EStyle Style, float pos, u32 Color)
    {
        SMarker NewMarker;
        NewMarker.m_dwColor = Color;
        NewMarker.m_eStyle = Style;
        NewMarker.m_fPos = pos;

        m_Markers.push_back(NewMarker);
    };

    const SMarker& Marker(u32 ID)
    {
        VERIFY(ID < m_Markers.size());
        return m_Markers[ID];
    };

    void UpdateMarkerPos(u32 ID, float NewPos)
    {
        if (ID >= m_Markers.size())
            return;
        SMarker& pMarker = m_Markers[ID];
        pMarker.m_fPos = NewPos;
    };
    void ClearMarkers() { m_Markers.clear(); }

    void RemoveMarker(u32 ID)
    {
        if (ID >= m_Markers.size())
            return;
        m_Markers.erase(m_Markers.begin() + ID);
    }
};
#endif
