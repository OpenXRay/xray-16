#include "StdAfx.h"
#include "HUDManager.h"
#include "HUDTarget.h"
#include "Actor.h"
#include "xrEngine/IGame_Level.h"
#include "xrEngine/xr_input.h"
#include "GamePersistent.h"
#include "MainMenu.h"
#include "Grenade.h"
#include "Spectator.h"
#include "Car.h"
#include "UIGameCustom.h"
#include "xrUICore/Cursor/UICursor.h"
#include "game_cl_base.h"
#ifdef DEBUG
#include "PHDebug.h"
#endif

extern CUIGameCustom* CurrentGameUI() { return HUD().GetGameUI(); }

//--------------------------------------------------------------------
CHUDManager::CHUDManager() : m_pHUDTarget(xr_new<CHUDTarget>()) {}
//--------------------------------------------------------------------
CHUDManager::~CHUDManager()
{
    OnDisconnected();

    if (pUIGame)
        pUIGame->UnLoad();

    xr_delete(pUIGame);
    xr_delete(m_pHUDTarget);
}

//--------------------------------------------------------------------
void CHUDManager::OnFrame()
{
    ZoneScoped;

    if (!psHUD_Flags.is(HUD_DRAW_RT2))
        return;

    if (!b_online)
        return;

    if (pUIGame)
        pUIGame->OnFrame();

    m_pHUDTarget->CursorOnFrame();
}
//--------------------------------------------------------------------

void CHUDManager::Render_First(u32 context_id)
{
    ZoneScoped;

    if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2 | HUD_DRAW_RT2))
        return;
    if (0 == pUIGame)
        return;
    IGameObject* O = g_pGameLevel->CurrentViewEntity();
    if (0 == O)
        return;
    CActor* A = smart_cast<CActor*>(O);
    if (!A || !A->HUDview())
        return;

    // On R1 render only shadow
    // On R2+ render everything
    {
        const auto root = O->H_Root();
        ScopeLock lock{ &render_lock };
        root->renderable_Invisible(GEnv.Render->GenerationIsR1());
        O->renderable_Render(context_id, root);
        root->renderable_Invisible(false);
    }
}

bool need_render_hud()
{
    if (Device.IsAnselActive)
        return false;

    IGameObject* O = g_pGameLevel ? g_pGameLevel->CurrentViewEntity() : NULL;
    if (0 == O)
        return false;

    CActor* A = smart_cast<CActor*>(O);
    if (A && (!A->HUDview() || !A->g_Alive()))
        return false;

    if (smart_cast<CCar*>(O) || smart_cast<CSpectator*>(O))
        return false;

    return true;
}

void CHUDManager::Render_Last(u32 context_id)
{
    ZoneScoped;

    if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2 | HUD_DRAW_RT2))
        return;
    if (0 == pUIGame)
        return;

    if (!need_render_hud())
        return;

    IGameObject* O = g_pGameLevel->CurrentViewEntity();
    // hud itself
    {
        const auto root = O->H_Root();
        ScopeLock lock{ &render_lock };
        root->renderable_HUD(true);
        O->OnHUDDraw(context_id, this, root);
        root->renderable_HUD(false);
    }
}

#include "player_hud.h"
bool CHUDManager::RenderActiveItemUIQuery()
{
    if (!psHUD_Flags.is(HUD_DRAW_RT2))
        return false;

    if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2))
        return false;

    if (!need_render_hud())
        return false;

    return (g_player_hud && g_player_hud->render_item_ui_query());
}

void CHUDManager::RenderActiveItemUI()
{
    if (!psHUD_Flags.is(HUD_DRAW_RT2))
        return;

    g_player_hud->render_item_ui();
}

extern ENGINE_API bool bShowPauseString;
//отрисовка элементов интерфейса
void CHUDManager::RenderUI()
{
    ZoneScoped;

    if (!psHUD_Flags.is(HUD_DRAW_RT2))
        return;

    if (!b_online)
        return;

    if (true /*|| psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT)*/)
    {
        HitMarker.Render();
        if (pUIGame)
            pUIGame->Render();

        UI().RenderFont();
    }

    m_pHUDTarget->Render();

    if (Device.Paused() && bShowPauseString)
    {
        CGameFont* pFont = UI().Font().pFontGraffiti50Russian;
        pFont->SetColor(0x80FF0000);
        LPCSTR _str = StringTable().translate("st_game_paused").c_str();

        Fvector2 _pos;
        _pos.set(UI_BASE_WIDTH / 2.0f, UI_BASE_HEIGHT / 2.0f);
        UI().ClientToScreenScaled(_pos);
        pFont->SetAligment(CGameFont::alCenter);
        pFont->Out(_pos.x, _pos.y, _str);
        pFont->OnRender();
    }
}

void CHUDManager::OnEvent(EVENT E, u64 P1, u64 P2) {}
collide::rq_result& CHUDManager::GetCurrentRayQuery() { return m_pHUDTarget->GetRQ(); }
void CHUDManager::SetCrosshairDisp(float dispf, float disps)
{
    m_pHUDTarget->GetHUDCrosshair().SetDispersion(psHUD_Flags.test(HUD_CROSSHAIR_DYNAMIC) ? dispf : disps);
}

#ifdef DEBUG
void CHUDManager::SetFirstBulletCrosshairDisp(float fbdispf)
{
    m_pHUDTarget->GetHUDCrosshair().SetFirstBulletDispertion(fbdispf);
}
#endif

void CHUDManager::ShowCrosshair(bool show) { m_pHUDTarget->ShowCrosshair(show); }
void CHUDManager::HitMarked(const Fvector& dir)
{
    HitMarker.Hit(dir);
}

bool CHUDManager::AddGrenade_ForMark(CGrenade* grn) { return HitMarker.AddGrenade_ForMark(grn); }
void CHUDManager::Update_GrenadeView(Fvector& pos_actor) { HitMarker.Update_GrenadeView(pos_actor); }
void CHUDManager::SetHitmarkType(LPCSTR tex_name) { HitMarker.InitShader(tex_name); }
void CHUDManager::SetGrenadeMarkType(LPCSTR tex_name) { HitMarker.InitShader_Grenade(tex_name); }
// ------------------------------------------------------------------------------------

void CHUDManager::Load()
{
    ZoneScoped;

    if (!pUIGame)
    {
        pUIGame = Game().createGameUI();
    }
    else
    {
        pUIGame->SetClGame(&Game());
    }
}

void CHUDManager::OnUIReset()
{
    ZoneScoped;

    pUIGame->HideShownDialogs();

    pUIGame->UnLoad();
    pUIGame->Load();

    pUIGame->OnConnected();
}

void CHUDManager::OnDisconnected()
{
    ZoneScoped;

    b_online = false;
    if (pUIGame)
        Device.seqFrame.Remove(pUIGame);
}

void CHUDManager::OnConnected()
{
    if (b_online)
        return;

    ZoneScoped;

    b_online = true;
    if (pUIGame)
        Device.seqFrame.Add(pUIGame, REG_PRIORITY_LOW - 1000);
}

void CHUDManager::net_Relcase(IGameObject* obj)
{
    ZoneScoped;

    HitMarker.net_Relcase(obj);

    VERIFY(m_pHUDTarget);
    m_pHUDTarget->net_Relcase(obj);
#ifdef DEBUG
    DBG_PH_NetRelcase(obj);
#endif
}

CDialogHolder* CurrentDialogHolder()
{
    if (MainMenu()->IsActive())
        return MainMenu();
    else
        return HUD().GetGameUI();
}
