#include "StdAfx.h"
#include "Level.h"
#include "UIGameDM.h"
#include "xrServer.h"
#include "game_sv_mp.h"
#include "Spectator.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "game_cl_mp.h"
#include "xrCore/stream_reader.h"
#include "Message_Filter.h"
#include "DemoPlay_Control.h"
#include "DemoInfo.h"
#include "xrEngine/CameraManager.h"

void CLevel::PrepareToSaveDemo()
{
    R_ASSERT(!m_DemoPlay);
    string_path demo_name = "";
    string_path demo_path;
#ifndef LINUX // FIXME!!!
    SYSTEMTIME Time;
    GetLocalTime(&Time);
    xr_sprintf(demo_name, "xray_%02d-%02d-%02d_%02d-%02d-%02d.demo", Time.wMonth, Time.wDay, Time.wYear, Time.wHour,
        Time.wMinute, Time.wSecond);
#endif
    Msg("Demo would be stored in - %s", demo_name);
    FS.update_path(demo_path, "$logs$", demo_name);
    m_writer = FS.w_open(demo_path);
    m_DemoSave = TRUE;
}

bool CLevel::PrepareToPlayDemo(shared_str const& file_name)
{
    R_ASSERT(!m_DemoSave);
    m_reader = FS.rs_open("$logs$", file_name.c_str());
    if (!m_reader)
    {
        Msg("ERROR: failed to open file [%s] to play demo...", file_name.c_str());
        return false;
    }
    if (!LoadDemoHeader())
    {
        Msg("ERROR: bad demo file...");
        return false;
    }
    m_DemoPlay = TRUE;
    return true;
}

void CLevel::StopSaveDemo()
{
    if (m_writer)
    {
        FS.w_close(m_writer);
    }
}

void CLevel::StartPlayDemo()
{
    R_ASSERT(IsDemoPlay() && !m_DemoPlayStarted);

    m_current_spectator = NULL;
    m_DemoPlayStarted = TRUE;
    m_StartGlobalTime = Device.dwTimeGlobal;
    SetDemoPlaySpeed(1.0f);
    m_starting_spawns_pos = 0;
    m_starting_spawns_dtime = 0;
    Msg("! ------------- Demo Started ------------");
    CatchStartingSpawns();

// if using some filter ...
#ifdef MP_LOGGING
    message_filter* tmp_msg_filter = GetMessageFilter();
    if (tmp_msg_filter)
    {
        string_path demo_msg_path;
        FS.update_path(demo_msg_path, "$logs$", "dbg_msg.log");
        tmp_msg_filter->dbg_set_message_log_file(demo_msg_path);
    }
#endif //#ifdef MP_LOGGING
}

void CLevel::RestartPlayDemo()
{
    if (!IsDemoPlay() || (m_starting_spawns_pos == 0))
    {
        Msg("! ERROR: no demo play started");
        return;
    }
    if (IsDemoPlayStarted())
    {
        remove_objects(); // WARNING ! need to be in DemoPlayStarted mode .
// After remove_objects() invokation there where left a serveral (20) UpdateCLs so:
#ifdef DEBUG
        VERIFY(g_pGameLevel);
        VERIFY(m_current_spectator);
        g_pGameLevel->Cameras().dbg_upd_frame = 0;
        m_current_spectator->SetDbgUpdateFrame(0);
#endif
        StopPlayDemo();
    }
    R_ASSERT(m_reader);

    m_DemoPlayStarted = TRUE;
    m_DemoPlayStoped = FALSE;

    m_StartGlobalTime = Device.dwTimeGlobal - m_starting_spawns_dtime;
    m_reader->seek(m_starting_spawns_pos);

    SetDemoPlaySpeed(1.0f);
    Msg("! ------------- Demo ReStarted ------------");
}

void CLevel::StopPlayDemo()
{
    SetDemoPlaySpeed(1.0f);
    if (m_reader)
    {
        // FS.r_close			(m_reader);
        // m_reader			= NULL;
        m_DemoPlayStarted = FALSE;
        m_DemoPlayStoped = TRUE;
    }
    Msg("! ------------- Demo Stoped ------------");
}

void CLevel::StartSaveDemo(shared_str const& server_options)
{
    R_ASSERT(IsDemoSave() && !m_DemoSaveStarted);

    SaveDemoHeader(server_options);
    m_DemoSaveStarted = TRUE;
}

void CLevel::SaveDemoHeader(shared_str const& server_options)
{
    m_demo_header.m_time_global = Device.dwTimeGlobal;
    m_demo_header.m_time_server = timeServer();
    m_demo_header.m_time_delta = timeServer_Delta();
    m_demo_header.m_time_delta_user = net_TimeDelta_User;
    m_writer->w(&m_demo_header, sizeof(m_demo_header));
    m_writer->w_stringZ(server_options);
    m_demo_info_file_pos = m_writer->tell();
    m_writer->seek(m_demo_info_file_pos + demo_info::max_demo_info_size);
}

void CLevel::SaveDemoInfo()
{
    game_cl_mp* tmp_game = smart_cast<game_cl_mp*>(&Game());
    if (!tmp_game)
        return;

    R_ASSERT(m_writer);

    u32 old_pos = m_writer->tell();
    m_writer->seek(m_demo_info_file_pos);
    if (!m_demo_info)
    {
        m_demo_info = new demo_info();
    }
    m_demo_info->load_from_game();
    m_demo_info->write_to_file(m_writer);
    m_writer->seek(old_pos);
}

void CLevel::SavePacket(NET_Packet& packet)
{
    m_writer->w_u32(Device.dwTimeGlobal - m_demo_header.m_time_global);
    m_writer->w_u32(packet.timeReceive);
    m_writer->w_u32(packet.B.count);
    m_writer->w(packet.B.data, packet.B.count);
}

bool CLevel::LoadDemoHeader()
{
    R_ASSERT(m_reader);
    m_reader->r(&m_demo_header, sizeof(m_demo_header));
    m_reader->r_stringZ(m_demo_server_options);
    u32 demo_info_start_pos = m_reader->tell();

    R_ASSERT(m_demo_info == NULL);
    m_demo_info = new demo_info();
    m_demo_info->read_from_file(m_reader);

    m_reader->seek(demo_info_start_pos + demo_info::max_demo_info_size);
    return (m_reader->elapsed() >= sizeof(DemoPacket));
}

bool CLevel::LoadPacket(NET_Packet& dest_packet, u32 global_time_delta)
{
    if (!m_reader || m_reader->eof())
        return false;

    m_prev_packet_pos = m_reader->tell();
    DemoPacket tmp_hdr;

    m_reader->r(&tmp_hdr, sizeof(DemoPacket));
    m_prev_packet_dtime = tmp_hdr.m_time_global_delta;

    if (map_data.m_sended_map_name_request ? /// ???
            (tmp_hdr.m_time_global_delta <= global_time_delta) :
            (tmp_hdr.m_time_global_delta < global_time_delta))
    {
        R_ASSERT2(tmp_hdr.m_packet_size < NET_PacketSizeLimit, "bad demo packet");
        m_reader->r(dest_packet.B.data, tmp_hdr.m_packet_size);
        dest_packet.B.count = tmp_hdr.m_packet_size;
        dest_packet.timeReceive = tmp_hdr.m_timeReceive; // not used ..
        dest_packet.r_pos = 0;
        if (m_reader->elapsed() <= sizeof(DemoPacket))
        {
            StopPlayDemo();
        }
        return true;
    }
    m_reader->seek(m_prev_packet_pos);
    return false;
}
void CLevel::SimulateServerUpdate()
{
    u32 tdelta = Device.dwTimeGlobal - m_StartGlobalTime;
    NET_Packet tmp_packet;
    while (LoadPacket(tmp_packet, tdelta))
    {
        if (m_msg_filter)
            m_msg_filter->check_new_data(tmp_packet);
        IPureClient::OnMessage(tmp_packet.B.data, tmp_packet.B.count);
    }
}

void CLevel::SpawnDemoSpectator()
{
    R_ASSERT(Server && Server->GetGameState());
    m_current_spectator = NULL;
    game_sv_mp* tmp_sv_game = smart_cast<game_sv_mp*>(Server->GetGameState());
    game_cl_mp* mp_cl_game = smart_cast<game_cl_mp*>(Level().game);

    CSE_Spectator* specentity = smart_cast<CSE_Spectator*>(tmp_sv_game->spawn_begin("spectator"));
    R_ASSERT(specentity);
    R_ASSERT2(mp_cl_game->local_player, "player not spawned");
    // mp_cl_game->local_player		= mp_cl_game->createPlayerState();
    // xr_strcpy						(mp_cl_game->local_player->name, "demo_spectator");
    specentity->set_name_replace(mp_cl_game->local_player->getName());
    specentity->s_flags.assign(M_SPAWN_OBJECT_LOCAL | M_SPAWN_OBJECT_ASPLAYER |
        M_SPAWN_OBJECT_PHANTOM); // M_SPAWN_OBJECT_PHANTOM is ONLY to indicate thath this is a fake spectator
    tmp_sv_game->assign_RP(specentity, Level().game->local_player);

    g_sv_Spawn(specentity);

    F_entity_Destroy(specentity);
}

void CLevel::SetDemoSpectator(IGameObject* spectator)
{
    R_ASSERT2(smart_cast<CSpectator*>(spectator), "tried to set not an spectator object to demo spectator");
    m_current_spectator = spectator;
}

float CLevel::GetDemoPlayPos() const
{
    // if (!m_reader)
    //	return 1.f;
    if (m_reader->eof())
        return 1.f;

    return (float(m_reader->tell()) / float(m_reader->length()));
}

message_filter* CLevel::GetMessageFilter()
{
    if (m_msg_filter)
        return m_msg_filter;
    m_msg_filter = new message_filter();
    return m_msg_filter;
}
demoplay_control* CLevel::GetDemoPlayControl()
{
    if (m_demoplay_control)
        return m_demoplay_control;
    m_demoplay_control = new demoplay_control();
    return m_demoplay_control;
}
/*
void CLevel::SetDemoPlayPos(float const pos)
{
    if (!IsDemoPlayStarted())
    {
        Msg("! ERROR: demo play not started");
        return;
    }
    if (pos > 1.f)
    {
        Msg("! ERROR: incorect demo play position");
        return;
    }
    float cur_pos = GetDemoPlayPos();
    if (cur_pos >= pos)
    {
        Msg("! demo play position must be greater than current position");
        return;
    }

    u32 old_file_pos = m_reader->tell();

    u32				file_pos = u32(float(m_reader->length()) * pos);
    if (file_pos <= old_file_pos)
    {
        Msg("! demo play position already at the current point");
        return;
    }

    DemoPacket		tmp_hdr;
    u32				time_shift = 0;

    while (m_reader->tell() < file_pos)
    {
        m_reader->r		(&tmp_hdr, sizeof(DemoPacket));
        m_reader->seek	(m_reader->tell() + tmp_hdr.m_packet_size);
        time_shift		+= tmp_hdr.m_time_global_delta;
    }
    m_StartGlobalTime	-= time_shift;
    m_rewind			= TRUE;
    m_reader->seek		(old_file_pos);
}*/

float CLevel::GetDemoPlaySpeed() const { return Device.time_factor(); }
#define MAX_PLAY_SPEED 8.f
void CLevel::SetDemoPlaySpeed(float const time_factor)
{
    if (!IsDemoPlayStarted())
    {
        Msg("! ERROR: demo play not started");
        return;
    }
    if (time_factor > MAX_PLAY_SPEED)
    {
        Msg("! Sorry, maximum play speed is: %1.1f", MAX_PLAY_SPEED);
        return;
    }
    Device.time_factor(time_factor);
}

void CLevel::CatchStartingSpawns()
{
    message_filter::msg_type_subtype_func_t spawns_catcher =
        fastdelegate::MakeDelegate(this, &CLevel::MSpawnsCatchCallback);
    message_filter* tmp_msg_filter = GetMessageFilter();
    R_ASSERT(tmp_msg_filter);
    u32 fake_sub_msg = 0;
    tmp_msg_filter->filter(M_SPAWN, fake_sub_msg, spawns_catcher);
}

void __stdcall CLevel::MSpawnsCatchCallback(u32 message, u32 subtype, NET_Packet& packet)
{
    // see SimulateServerUpdate and using of message_filter
    m_starting_spawns_pos = m_prev_packet_pos;
    m_starting_spawns_dtime = m_prev_packet_dtime;
    u32 fake_sub_msg = 0;
    message_filter* tmp_msg_filter = GetMessageFilter();
    R_ASSERT(tmp_msg_filter);
    tmp_msg_filter->remove_filter(M_SPAWN, fake_sub_msg);
}

IGameObject* CLevel::GetDemoSpectator() { return smart_cast<CGameObject*>(m_current_spectator); };
