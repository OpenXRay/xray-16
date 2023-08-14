#pragma once

#include "xrEngine/CustomHUD.h"
#include "HitMarker.h"

class CHUDTarget;
class CUIGameCustom;
namespace collide { struct rq_result; }

class CHUDManager final : public CCustomHUD
{
    Lock render_lock{}; // TODO: I believe this can be avoided, need to think more about it
    CUIGameCustom* pUIGame{};
    CHitMarker HitMarker;
    CHUDTarget* m_pHUDTarget;
    bool b_online{};

public:
    CHUDManager();
    virtual ~CHUDManager();
    virtual void OnEvent(EVENT E, u64 P1, u64 P2);

    virtual void Render_First(u32 context_id);
    virtual void Render_Last(u32 context_id);
    virtual void OnFrame();

    virtual void RenderUI();

    CUIGameCustom* GetGameUI() { return pUIGame; }
    void HitMarked(const Fvector& dir);
    bool AddGrenade_ForMark(CGrenade* grn);
    void Update_GrenadeView(Fvector& pos_actor);
    void net_Relcase(IGameObject* obj);

    //текущий предмет на который смотрит HUD
    collide::rq_result& GetCurrentRayQuery();

    //устанвка внешнего вида прицела в зависимости от текущей дисперсии
    void SetCrosshairDisp(float dispf, float disps = 0.f);
#ifdef DEBUG
    void SetFirstBulletCrosshairDisp(float fbdispf);
#endif
    void ShowCrosshair(bool show);

    void SetHitmarkType(LPCSTR tex_name);
    void SetGrenadeMarkType(LPCSTR tex_name);

    void OnUIReset() override;

    virtual void Load();
    virtual void OnDisconnected();
    virtual void OnConnected();

    virtual void RenderActiveItemUI();
    virtual bool RenderActiveItemUIQuery();

    // Lain: added
    void SetRenderable(bool renderable) { psHUD_Flags.set(HUD_DRAW_RT2, renderable); }
};

IC CHUDManager& HUD() { return *static_cast<CHUDManager*>(Level().pHUD); }
