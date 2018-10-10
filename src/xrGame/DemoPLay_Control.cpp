#include "StdAfx.h"
#include "DemoPlay_Control.h"
#include "Level.h"
#include "game_cl_base.h"

demoplay_control::user_callback_t demoplay_control::no_user_callback;

demoplay_control::demoplay_control()
{
    m_onround_start = fastdelegate::MakeDelegate(this, &demoplay_control::on_round_start_impl);
    m_on_kill = fastdelegate::MakeDelegate(this, &demoplay_control::on_kill_impl);
    m_on_die = fastdelegate::MakeDelegate(this, &demoplay_control::on_die_impl);
    m_on_artefactdelivering = fastdelegate::MakeDelegate(this, &demoplay_control::on_artefactdelivering_impl);
    m_on_artefactcapturing = fastdelegate::MakeDelegate(this, &demoplay_control::on_artefactcapturing_impl);
    m_on_artefactloosing = fastdelegate::MakeDelegate(this, &demoplay_control::on_artefactloosing_impl);
    m_current_mode = not_active;
}

demoplay_control::~demoplay_control() {}
void demoplay_control::pause_on(EAction const action, shared_str const& param)
{
    if (m_current_mode != not_active)
    {
        Msg("! ERROR: already active.");
        return;
    }
    if (Device.Paused())
    {
        Device.Pause(FALSE, TRUE, TRUE, "playing demo until");
    }
    m_current_mode = waiting_for_actions;
    activate_filer(action, param);
}

void demoplay_control::cancel_pause_on()
{
    if (m_current_mode != waiting_for_actions)
    {
        Msg("! ERROR: pause on is not active");
        return;
    }
    deactivate_filter();
    m_current_mode = not_active;
}

const float demoplay_control::rewind_speed = 8.f;
bool demoplay_control::rewind_until(EAction const action, shared_str const& param, user_callback_t ucb)
{
    if (m_current_mode != not_active)
    {
        Msg("! ERROR: already active.");
        return false;
    }
    if (Device.Paused())
    {
        Device.Pause(FALSE, TRUE, TRUE, "playing demo until");
    }
    m_prev_speed = Level().GetDemoPlaySpeed();
    m_current_mode = rewinding;
    activate_filer(action, param);
    m_user_callback = ucb;
    Level().SetDemoPlaySpeed(rewind_speed);
    return true;
}
void demoplay_control::stop_rewind()
{
    if (m_current_mode != rewinding)
    {
        return;
    }
    deactivate_filter();
    Level().SetDemoPlaySpeed(m_prev_speed);
    m_current_mode = not_active;
}

void demoplay_control::activate_filer(EAction const action, shared_str const& param)
{
    m_action_param_str = param;
    m_current_action = action;
    message_filter* tmp_msg_filter = Level().GetMessageFilter();
    R_ASSERT2(tmp_msg_filter, "can't get message filter object");

    switch (action)
    {
    case on_round_start: { tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_ROUND_STARTED, m_onround_start);
    }
    break;
    case on_kill: { tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_PLAYER_KILLED, m_on_kill);
    }
    break;
    case on_die: { tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_PLAYER_KILLED, m_on_die);
    }
    break;
    case on_artefactdelivering:
    {
        tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_ONBASE, m_on_artefactdelivering);
    }
    break;
    case on_artefactcapturing:
    {
        tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_TAKEN, m_on_artefactcapturing);
    }
    break;
    case on_artefactloosing: { tmp_msg_filter->filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_DROPPED, m_on_artefactloosing);
    }
    break;
    default: { FATAL("unknown action to filter");
    }
    }; // switch (action)
}
void demoplay_control::deactivate_filter()
{
    message_filter* tmp_msg_filter = Level().GetMessageFilter();
    R_ASSERT2(tmp_msg_filter, "can't get message filter object");

    switch (m_current_action)
    {
    case on_round_start: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_ROUND_STARTED);
    }
    break;
    case on_kill: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_PLAYER_KILLED);
    }
    break;
    case on_die: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_PLAYER_KILLED);
    }
    break;
    case on_artefactdelivering: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_ONBASE);
    }
    break;
    case on_artefactcapturing: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_TAKEN);
    }
    break;
    case on_artefactloosing: { tmp_msg_filter->remove_filter(M_GAMEMESSAGE, GAME_EVENT_ARTEFACT_DROPPED);
    }
    break;
    default: { FATAL("unknown action to remove filter");
    }
    }; // switch (action)
}
void demoplay_control::process_action()
{
    if (m_current_mode == rewinding)
    {
        Level().SetDemoPlaySpeed(m_prev_speed);
    }
    Device.Pause(TRUE, TRUE, TRUE, "game action captured");
    deactivate_filter();
    if (m_user_callback)
        m_user_callback();
    m_current_mode = not_active;
}

void __stdcall demoplay_control::on_round_start_impl(u32 message, u32 subtype, NET_Packet& packet) { process_action(); }
void __stdcall demoplay_control::on_kill_impl(u32 message, u32 subtype, NET_Packet& packet)
{
    u16 msg_type;
    packet.r_begin(msg_type);
    R_ASSERT(msg_type == M_GAMEMESSAGE);
    u32 game_msg_type;
    packet.r_u32(game_msg_type);
    R_ASSERT(game_msg_type == GAME_EVENT_PLAYER_KILLED);

    if (!m_action_param_str.size())
    {
        process_action();
        return;
    }
    packet.r_u8(); // kill type
    packet.r_u16(); // killed_id
    u16 killer_id = packet.r_u16();
    game_PlayerState* killerps = Game().GetPlayerByGameID(killer_id);
    if (!killerps)
        return;

    if (strstr(killerps->getName(), m_action_param_str.c_str()))
    {
        process_action();
        return;
    }
}
void __stdcall demoplay_control::on_die_impl(u32 message, u32 subtype, NET_Packet& packet)
{
    u16 msg_type;
    packet.r_begin(msg_type);
    R_ASSERT(msg_type == M_GAMEMESSAGE);
    u32 game_msg_type;
    packet.r_u32(game_msg_type);
    R_ASSERT(game_msg_type == GAME_EVENT_PLAYER_KILLED);

    if (!m_action_param_str.size())
    {
        process_action();
        return;
    }
    packet.r_u8(); // kill type
    u16 killed_id = packet.r_u16();
    game_PlayerState* killedps = Game().GetPlayerByGameID(killed_id);
    if (!killedps)
        return;

    if (strstr(killedps->getName(), m_action_param_str.c_str()))
    {
        process_action();
        return;
    }
}
void __stdcall demoplay_control::on_artefactdelivering_impl(u32 message, u32 subtype, NET_Packet& packet)
{
    u16 msg_type;
    packet.r_begin(msg_type);
    R_ASSERT(msg_type == M_GAMEMESSAGE);
    u32 game_msg_type;
    packet.r_u32(game_msg_type);
    R_ASSERT(game_msg_type == GAME_EVENT_ARTEFACT_ONBASE);

    if (!m_action_param_str.size())
    {
        process_action();
        return;
    }
    EGameIDs current_game_type = static_cast<EGameIDs>(GameID());
    u16 deliverer_id = 0; // who deliver the artefact
    if (current_game_type == eGameIDCaptureTheArtefact)
    {
        u8 delivererTeam;
        packet.r_u8(delivererTeam);
        packet.r_u16(deliverer_id);
    }
    else if (current_game_type == eGameIDArtefactHunt)
    {
        packet.r_u16(deliverer_id);
    }
    else
    {
        FATAL("incorect event for current game type");
    }
    game_PlayerState* delivererps = Game().GetPlayerByGameID(deliverer_id);

    if (!delivererps)
        return;

    if (strstr(delivererps->getName(), m_action_param_str.c_str()))
    {
        process_action();
        return;
    }
}

void __stdcall demoplay_control::on_artefactcapturing_impl(u32 message, u32 subtype, NET_Packet& packet)
{
    u16 msg_type;
    packet.r_begin(msg_type);
    R_ASSERT(msg_type == M_GAMEMESSAGE);
    u32 game_msg_type;
    packet.r_u32(game_msg_type);
    R_ASSERT(game_msg_type == GAME_EVENT_ARTEFACT_TAKEN);

    if (!m_action_param_str.size())
    {
        process_action();
        return;
    }
    EGameIDs current_game_type = static_cast<EGameIDs>(GameID());
    game_PlayerState* capturerps = NULL;
    if (current_game_type == eGameIDCaptureTheArtefact)
    {
        u8 capturer_team;
        packet.r_u8(capturer_team);
        ClientID clientId;
        packet.r_clientID(clientId);
        game_cl_GameState::PLAYERS_MAP_CIT playerIt = Game().players.find(clientId);
        if (playerIt != Game().players.end())
        {
            capturerps = playerIt->second;
        }
    }
    else if (current_game_type == eGameIDArtefactHunt)
    {
        u16 capturer_id;
        packet.r_u16(capturer_id);
        capturerps = Game().GetPlayerByGameID(capturer_id);
    }
    else
    {
        FATAL("incorect message for current game type");
    }

    if (!capturerps)
        return;

    if (strstr(capturerps->getName(), m_action_param_str.c_str()))
    {
        process_action();
        return;
    }
}
void __stdcall demoplay_control::on_artefactloosing_impl(u32 message, u32 subtype, NET_Packet& packet)
{
    u16 msg_type;
    packet.r_begin(msg_type);
    R_ASSERT(msg_type == M_GAMEMESSAGE);
    u32 game_msg_type;
    packet.r_u32(game_msg_type);
    R_ASSERT(game_msg_type == GAME_EVENT_ARTEFACT_DROPPED);

    if (!m_action_param_str.size())
    {
        process_action();
        return;
    }
    EGameIDs current_game_type = static_cast<EGameIDs>(GameID());
    game_PlayerState* looserps = NULL;
    if (current_game_type == eGameIDCaptureTheArtefact)
    {
        u8 capturer_team;
        packet.r_u8(capturer_team);
        ClientID clientId;
        packet.r_clientID(clientId);
        game_cl_GameState::PLAYERS_MAP_CIT playerIt = Game().players.find(clientId);
        if (playerIt != Game().players.end())
        {
            looserps = playerIt->second;
        }
    }
    else if (current_game_type == eGameIDArtefactHunt)
    {
        u16 looser_id;
        packet.r_u16(looser_id);
        looserps = Game().GetPlayerByGameID(looser_id);
    }
    else
    {
        FATAL("incorect message for current game type");
    }

    if (!looserps)
        return;

    if (strstr(looserps->getName(), m_action_param_str.c_str()))
    {
        process_action();
        return;
    }
}
