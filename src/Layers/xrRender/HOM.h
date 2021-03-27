// HOM.h: interface for the CHOM class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/Render.h"

class occTri;

class CHOM
#ifdef DEBUG
    : public pureRender
#endif
{
private:
    struct HOMStatistics
    {
        CStatTimer Total;
        Lock TotalTimerLock;
        u32 FrustumTriangleCount;
        u32 VisibleTriangleCount;

        HOMStatistics() { FrameStart(); }
        void FrameStart()
        {
            Total.FrameStart();
            FrustumTriangleCount = 0;
            VisibleTriangleCount = 0;
        }

        void FrameEnd() { Total.FrameEnd(); }
    };

    xrXRC xrc;
    CDB::MODEL* m_pModel;
    occTri* m_pTris;
    BOOL bEnabled;
    Fmatrix m_xform;
    Fmatrix m_xform_01;

    mutable HOMStatistics stats;

    void Render_DB(CFrustum& base);

public:
    void Load();
    void Unload();
    void Render(CFrustum& base);
    void Render_ZB();
    //	void					Debug		();

    void occlude(Fbox2& /*space*/) {}
    void Disable();
    void Enable();

    void xr_stdcall MT_RENDER(Task& /*thisTask*/, void* /*data*/);

    BOOL visible(vis_data& vis) const;
    BOOL visible(const Fbox3& B) const;
    BOOL visible(const sPoly& P) const;
    BOOL visible(const Fbox2& B, float depth) const; // viewport-space (0..1)

    CHOM();
    ~CHOM();

    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
#ifdef DEBUG
    virtual void OnRender();
#endif
};
