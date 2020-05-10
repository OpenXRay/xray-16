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

    Lock MT;
    volatile u32 MT_frame_rendered;
    HOMStatistics stats;

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

    bool MT_Synced() const
    {
        return IGame_Persistent::MainMenuActiveOrLevelNotExist();
    }

    void __stdcall MT_RENDER();
    ICF void MT_Sync()
    {
        if (MT_Synced())
            return;
        MT_RENDER();
    }

    BOOL visible(vis_data& vis);
    BOOL visible(Fbox3& B);
    BOOL visible(sPoly& P);
    BOOL visible(Fbox2& B, float depth); // viewport-space (0..1)

    CHOM();
    ~CHOM();

    void DumpStatistics(class IGameFont& font, class IPerformanceAlert* alert);
#ifdef DEBUG
    virtual void OnRender();
#endif
};
