#pragma once
#include "xrEngine/EngineAPI.h"
#include "xrEngine/EventAPI.h"
#include "xrEngine/pure.h"

ENGINE_API extern Flags32 psHUD_Flags;
#define HUD_CROSSHAIR (1 << 0)
#define HUD_CROSSHAIR_DIST (1 << 1)
#define HUD_WEAPON (1 << 2)
#define HUD_INFO (1 << 3)
#define HUD_DRAW (1 << 4)
#define HUD_CROSSHAIR_RT (1 << 5)
#define HUD_WEAPON_RT (1 << 6)
#define HUD_CROSSHAIR_DYNAMIC (1 << 7)
#define HUD_CROSSHAIR_RT2 (1 << 9)
#define HUD_DRAW_RT (1 << 10)
#define HUD_WEAPON_RT2 (1 << 11)
#define HUD_DRAW_RT2 (1 << 12)

class IGameObject;

class ENGINE_API CCustomHUD : public FactoryObjectBase, public IEventReceiver,
                              public CUIResetAndResolutionNotifier
{
public:
    CCustomHUD();
    virtual ~CCustomHUD();

    BENCH_SEC_SCRAMBLEVTBL2

    virtual void Render_First() { ; }
    virtual void Render_Last() { ; }
    virtual void Render_Actor_Shadow() {}
    BENCH_SEC_SCRAMBLEVTBL1

    virtual void OnFrame() { ; }
    virtual void OnEvent(EVENT /*E*/, u64 /*P1*/, u64 /*P2*/) { ; }
    virtual void Load() { ; }
    virtual void OnDisconnected() = 0;
    virtual void OnConnected() = 0;
    virtual void RenderActiveItemUI() = 0;
    virtual bool RenderActiveItemUIQuery() = 0;
    virtual void net_Relcase(IGameObject* object) = 0;
};

extern ENGINE_API CCustomHUD* g_hud;
