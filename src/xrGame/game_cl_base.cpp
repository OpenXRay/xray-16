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

#include "xrNetServer/NET_Messages.h"

EGameIDs ParseStringToGameType(LPCSTR str);
LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

game_cl_GameState::game_cl_GameState()
{
	local_player				= createPlayerState(NULL);	//initializing account info

    m_game_type_name = 0;

    shedule.t_min = 5;
    shedule.t_max = 20;
    m_game_ui_custom = NULL;
    shedule_register();

	m_u16VotingEnabled			= 0;
	m_bServerControlHits		= true;
}

game_cl_GameState::~game_cl_GameState()
{
    PLAYERS_MAP_IT I = players.begin();
    for (; I != players.end(); ++I)
        xr_delete(I->second);
    players.clear();

	shedule_unregister();
	xr_delete(local_player);
}

void game_cl_GameState::net_import_GameTime(NET_Packet& P)
{
}

void game_cl_GameState::net_import_state(NET_Packet& P)
{
}

void	game_cl_GameState::net_import_update(NET_Packet& P)
{
}

void game_cl_GameState::net_signal(NET_Packet& P) {}
void game_cl_GameState::TranslateGameMessage(u32 msg, NET_Packet& P)
{
    CStringTable st;

    string512 Text;
    constexpr char Color_Main[] = "%c[255,192,192,192]";
    constexpr pcstr Color_Teams[3] = {"%c[255,255,240,190]", "%c[255,64,255,64]", "%c[255,64,64,255]"};

    switch (msg)
    {
    case GAME_EVENT_PLAYER_CONNECTED:
    {
        ClientID newClientId;
        P.r_clientID(newClientId);
        game_PlayerState* PS = nullptr;
        if (newClientId == local_svdpnid)
        {
            PS = local_player;
        }
        else
        {
            PS = createPlayerState(&P);
        }
        VERIFY2(PS, "failed to create player state");

        if (Type() != eGameIDSingle)
        {
            players.insert(std::make_pair(newClientId, PS));
            OnNewPlayerConnected(newClientId);
        }
        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PS->getName(), Color_Main, *st.translate("mp_connected"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        //---------------------------------------
        Msg("%s connected", PS->getName());
    }
    break;
    case GAME_EVENT_PLAYER_DISCONNECTED:
    {
        string64 PlayerName;
        P.r_stringZ(PlayerName);

        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *st.translate("mp_disconnected"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
        //---------------------------------------
        Msg("%s disconnected", PlayerName);
    }
    break;
    case GAME_EVENT_PLAYER_ENTERED_GAME:
    {
        string64 PlayerName;
        P.r_stringZ(PlayerName);

        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *st.translate("mp_entered_game"));
        if (CurrentGameUI())
            CurrentGameUI()->CommonMessageOut(Text);
    }
    break;
    default: { R_ASSERT2(0, "Unknown Game Message");
    }
    break;
    };
}

void game_cl_GameState::OnGameMessage(NET_Packet& P)
{
    VERIFY(this && &P);
    u32 msg;
    P.r_u32(msg);

    TranslateGameMessage(msg, P);
};

game_PlayerState* game_cl_GameState::lookat_player()
{
    IGameObject* current_entity = Level().CurrentEntity();
    if (current_entity)
    {
        return GetPlayerByGameID(current_entity->ID());
    }
    return NULL;
}

game_PlayerState* game_cl_GameState::GetPlayerByGameID(u32 GameID)
{
    PLAYERS_MAP_IT I = players.begin();
    PLAYERS_MAP_IT E = players.end();

    for (; I != E; ++I)
    {
        game_PlayerState* P = I->second;
        if (P->GameID == GameID)
            return P;
    };
    return NULL;
};

game_PlayerState* game_cl_GameState::GetPlayerByOrderID(u32 idx)
{
    PLAYERS_MAP_IT I = players.begin();
    std::advance(I, idx);
    game_PlayerState* ps = I->second;
    return ps;
}

ClientID game_cl_GameState::GetClientIDByOrderID(u32 idx)
{
    PLAYERS_MAP_IT I = players.begin();
    std::advance(I, idx);
    return I->first;
}

void game_cl_GameState::shedule_Update(u32 dt)
{
    ScheduledBase::shedule_Update(dt);

    if (!m_game_ui_custom)
    {
        if (CurrentGameUI())
            m_game_ui_custom = CurrentGameUI();
    }
    //---------------------------------------
    switch (Phase())
    {
    case GAME_PHASE_INPROGRESS:
    {
        
    }
    break;
    default: {
    }
    break;
    };
};

void game_cl_GameState::sv_GameEventGen(NET_Packet& P)
{
    P.w_begin(M_EVENT);
    P.w_u32(Level().timeServer());
    P.w_u16(u16(GE_GAME_EVENT & 0xffff));
    P.w_u16(0); // dest==0
}

void game_cl_GameState::sv_EventSend(NET_Packet& P) { Level().Send(P, net_flags(TRUE, TRUE)); }
bool game_cl_GameState::OnKeyboardPress(int dik)
{
    if (!local_player || local_player->IsSkip())
        return true;
    else
        return false;
}

bool game_cl_GameState::OnKeyboardRelease(int dik)
{
    if (!local_player || local_player->IsSkip())
        return true;
    else
        return false;
}

void game_cl_GameState::u_EventGen(NET_Packet& P, u16 type, u16 dest)
{
    P.w_begin(M_EVENT);
    P.w_u32(Level().timeServer());
    P.w_u16(type);
    P.w_u16(dest);
}

void game_cl_GameState::u_EventSend(NET_Packet& P) { Level().Send(P, net_flags(TRUE, TRUE)); }
void game_cl_GameState::OnSwitchPhase(u32 old_phase, u32 new_phase)
{
    switch (old_phase)
    {
    case GAME_PHASE_INPROGRESS: {
    }
    break;
    default: {
    }
    break;
    };

	switch (new_phase)
	{
		case GAME_PHASE_INPROGRESS:
			{

			}break;
		default:
			{
			}break;
	}	
}

void game_cl_GameState::SendPickUpEvent(u16 ID_who, u16 ID_what)
{
    IGameObject* O = Level().Objects.net_Find(ID_what);
    Level().m_feel_deny.feel_touch_deny(O, 1000);

    NET_Packet P;
    u_EventGen(P, GE_OWNERSHIP_TAKE, ID_who);
    P.w_u16(ID_what);
    u_EventSend(P);
};

void game_cl_GameState::set_type_name(LPCSTR s)
{
    EGameIDs gid = ParseStringToGameType(s);
    m_game_type_name = GameTypeToString(gid, false);
    if (OnClient())
    {
        xr_strcpy(g_pGamePersistent->m_game_params.m_game_type, m_game_type_name.c_str());
        g_pGamePersistent->OnGameStart();
    }
};

void game_cl_GameState::OnConnected() { m_game_ui_custom = CurrentGameUI(); 	switch_Phase(GAME_PHASE_INPROGRESS);}
