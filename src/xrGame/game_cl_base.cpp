#include "pch_script.h"
#include "game_cl_base.h"
#include "Level.h"
#include "GamePersistent.h"
#include "UIGameCustom.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xr_Level_controller.h"
#include "ui/UIMainIngameWnd.h"
#include "UI/UIGameTutorial.h"
#include "UI/UIMessagesWindow.h"
#include "UI/UIDialogWnd.h"
#include "string_table.h"

LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

game_cl_GameState::game_cl_GameState()
{
    shedule.t_min = 5;
    shedule.t_max = 20;
    m_game_ui_custom = NULL;
    shedule_register();

	m_u16VotingEnabled			= 0;
	m_bServerControlHits		= true;
}

game_cl_GameState::~game_cl_GameState()
{
	shedule_unregister();
}

void game_cl_GameState::net_import_GameTime(NET_Packet& P)
{
}

void game_cl_GameState::net_import_state(NET_Packet& P)
{
}

void game_cl_GameState::net_import_update(NET_Packet& P)
{
}

void game_cl_GameState::net_signal(NET_Packet& P) {}

void game_cl_GameState::shedule_Update(u32 dt)
{
    ScheduledBase::shedule_Update(dt);

    if (!m_game_ui_custom)
    {
        if (CurrentGameUI())
            m_game_ui_custom = CurrentGameUI();
    }
};

bool game_cl_GameState::OnKeyboardPress(int dik)
{
    return false;
}

bool game_cl_GameState::OnKeyboardRelease(int dik)
{
    return false;
}

void game_cl_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
    P.w_begin(M_EVENT);
    P.w_u32(Level().timeServer());
    P.w_u16(type);
    P.w_u16(dest);
}

void game_cl_GameState::u_EventSend(NET_Packet& P) { Level().Send(P); }
void game_cl_GameState::OnSwitchPhase(u32 old_phase, u32 new_phase)
{}

void game_cl_GameState::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
    IGameObject* O = Level().Objects.net_Find(ID_what);
    Level().m_feel_deny.feel_touch_deny(O, 1000);

    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_TAKE, ID_who);
    P.w_u16(ID_what);
    u_EventSend(P);
};

void game_cl_GameState::OnConnected()
{
    m_game_ui_custom = CurrentGameUI();
    switch_Phase(GAME_PHASE_INPROGRESS);
}
