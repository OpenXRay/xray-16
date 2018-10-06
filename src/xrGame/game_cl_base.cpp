#include "pch_script.h"
#include "game_cl_base.h"
#include "Level.h"
#include "GamePersistent.h"
#include "UIGameCustom.h"
#include "xrScriptEngine/script_engine.hpp"
#include "xr_level_controller.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIGameTutorial.h"
#include "ui/UIMessagesWindow.h"
#include "ui/UIDialogWnd.h"
#include "string_table.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "game_sv_mp_vote_flags.h"
#include "xrNetServer/NET_Messages.h"

EGameIDs ParseStringToGameType(LPCSTR str);
LPCSTR GameTypeToString(EGameIDs gt, bool bShort);

game_cl_GameState::game_cl_GameState()
{
    local_player = createPlayerState(NULL); // initializing account info
    m_WeaponUsageStatistic = NULL;

    m_game_type_name = 0;

    shedule.t_min = 5;
    shedule.t_max = 20;
    m_game_ui_custom = NULL;
    shedule_register();

    m_u16VotingEnabled = 0;
    m_bServerControlHits = true;

    m_WeaponUsageStatistic = new WeaponUsageStatistic();
}

game_cl_GameState::~game_cl_GameState()
{
    PLAYERS_MAP_IT I = players.begin();
    for (; I != players.end(); ++I)
        xr_delete(I->second);
    players.clear();

    shedule_unregister();
    xr_delete(m_WeaponUsageStatistic);
    xr_delete(local_player);
}

void game_cl_GameState::net_import_GameTime(NET_Packet& P)
{
    u64 GameTime;
    P.r_u64(GameTime);
    float TimeFactor;
    P.r_float(TimeFactor);

    Level().SetGameTimeFactor(GameTime, TimeFactor);

    u64 GameEnvironmentTime;
    P.r_u64(GameEnvironmentTime);
    float EnvironmentTimeFactor;
    P.r_float(EnvironmentTimeFactor);

    u64 OldTime = Level().GetEnvironmentGameTime();
    Level().SetEnvironmentGameTimeFactor(GameEnvironmentTime, EnvironmentTimeFactor);
    if (OldTime > GameEnvironmentTime)
        GamePersistent().Environment().Invalidate();
}

struct not_exsiting_clients_deleter
{
    typedef buffer_vector<ClientID> existing_clients_vector_t;
    existing_clients_vector_t* exist_clients;
    game_PlayerState** local_player;
    ClientID* client_id;
    not_exsiting_clients_deleter(existing_clients_vector_t* exist, game_PlayerState** local_player, ClientID* client_id)
        : exist_clients(exist), local_player(local_player), client_id(client_id)
    {
    }
    // default copy constructor is right
    bool operator()(game_cl_GameState::PLAYERS_MAP::value_type& value)
    {
        VERIFY(exist_clients);
        existing_clients_vector_t::iterator tmp_iter = std::find(exist_clients->begin(), exist_clients->end(),
            value.first // key
            );

        if (tmp_iter != exist_clients->end())
            return false;

        if (*local_player == value.second)
            return false;
        // 			*local_player	=	NULL;
        // 			*client_id		=	0;
        // 		}

        xr_delete(value.second);
        return true;
    }
}; // not_present_clients_deleter

void game_cl_GameState::net_import_state(NET_Packet& P)
{
    // Generic
    P.r_clientID(local_svdpnid);
    P.r_u32((u32&)m_type);

    u16 ph;
    P.r_u16(ph);

    if (Phase() != ph)
        switch_Phase(ph);

    P.r_s32(m_round);
    P.r_u32(m_start_time);
    m_u16VotingEnabled = u16(P.r_u8());
    m_bServerControlHits = !!P.r_u8();
    m_WeaponUsageStatistic->SetCollectData(!!P.r_u8());

    // Players
    u16 p_count;
    P.r_u16(p_count);
    R_ASSERT(p_count <= MAX_PLAYERS_COUNT);

    buffer_vector<ClientID> valid_players(_alloca(sizeof(ClientID) * (p_count + 1)), (p_count + 1));

    for (u16 p_it = 0; p_it < p_count; ++p_it)
    {
        ClientID ID;
        P.r_clientID(ID);

        game_PlayerState* IP;
        PLAYERS_MAP_IT I = players.find(ID);
        if (I != players.end())
        {
            IP = I->second;
            //***********************************************
            u16 OldFlags = IP->flags__;
            u8 OldVote = IP->m_bCurrentVoteAgreed;
            //-----------------------------------------------
            IP->net_Import(P);
            //-----------------------------------------------
            if (OldFlags != IP->flags__)
                if (Type() != eGameIDSingle)
                    OnPlayerFlagsChanged(IP);
            if (OldVote != IP->m_bCurrentVoteAgreed)
                OnPlayerVoted(IP);
            //***********************************************
            valid_players.push_back(ID);
        }
        else
        {
            if (ID == local_svdpnid) // Level().GetClientID())
            {
                game_PlayerState::skip_Import(P); // this mean that local_player not created yet ..
                continue;
            }

            IP = createPlayerState(&P);

            if (Type() != eGameIDSingle)
                OnPlayerFlagsChanged(IP);

            players.insert(std::make_pair(ID, IP));
            valid_players.push_back(ID);
        }
    }
    not_exsiting_clients_deleter tmp_deleter(&valid_players, &local_player, &local_svdpnid);

    players.erase(std::remove_if(players.begin(), players.end(), tmp_deleter), players.end());

    net_import_GameTime(P);
}

void game_cl_GameState::net_import_update(NET_Packet& P)
{
    // Read
    ClientID ID;
    P.r_clientID(ID);

    // Update
    PLAYERS_MAP_IT I = players.find(ID);
    /*VERIFY2(I != players.end(),
        make_string("Player ClientID = %d not found in players map", ID.value()).c_str());*/
    if (players.end() != I)
    {
        game_PlayerState* IP = I->second;
        //		CopyMemory	(&IP,&PS,sizeof(PS));
        //***********************************************
        u16 OldFlags = IP->flags__;
        u8 OldVote = IP->m_bCurrentVoteAgreed;
        //-----------------------------------------------
        IP->net_Import(P);
        //-----------------------------------------------
        if (OldFlags != IP->flags__)
            if (Type() != eGameIDSingle)
                OnPlayerFlagsChanged(IP);
        if (OldVote != IP->m_bCurrentVoteAgreed)
            OnPlayerVoted(IP);
        //***********************************************
    }
    else
    {
        // updates can be delivered faster than guarantee packets
        // that store GAME_EVENT_PLAYER_CONNECTED
        game_PlayerState::skip_Import(P);
    };

    // Syncronize GameTime
    net_import_GameTime(P);
}

void game_cl_GameState::net_signal(NET_Packet& P) {}
void game_cl_GameState::TranslateGameMessage(u32 msg, NET_Packet& P)
{
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
        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PS->getName(), Color_Main, *StringTable().translate("mp_connected"));
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

        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *StringTable().translate("mp_disconnected"));
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

        xr_sprintf(Text, "%s%s %s%s", Color_Teams[0], PlayerName, Color_Main, *StringTable().translate("mp_entered_game"));
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
        if (!IsGameTypeSingle())
            m_WeaponUsageStatistic->Update();
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
    case GAME_PHASE_INPROGRESS: { m_WeaponUsageStatistic->Clear();
    }
    break;
    default: {
    }
    break;
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

void game_cl_GameState::OnConnected() { m_game_ui_custom = CurrentGameUI(); }
