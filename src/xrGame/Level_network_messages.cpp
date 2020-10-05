#include "StdAfx.h"
#include "Entity.h"
#include "xrServer_Objects.h"
#include "Level.h"
#include "xrMessages.h"
#include "game_cl_base.h"
#include "NET_Queue.h"

#include "xrServer.h"
#include "Actor.h"
#include "Artefact.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "ai_space.h"
#include "saved_game_wrapper.h"
#include "xrAICore/Navigation/level_graph.h"
#include "file_transfer.h"
#include "Message_Filter.h"
#include "xrPhysics/IPHWorld.h"

extern LPCSTR map_ver_string;
pstr remove_version_option(LPCSTR opt_str, pstr new_opt_str, u32 new_opt_str_size)
{
    LPCSTR temp_substr = strstr(opt_str, map_ver_string);
    if (!temp_substr)
    {
        xr_strcpy(new_opt_str, new_opt_str_size, opt_str);
        return new_opt_str;
    }
    strncpy_s(new_opt_str, new_opt_str_size, opt_str, static_cast<size_t>(temp_substr - opt_str - 1));
    temp_substr = strchr(temp_substr, '/');
    if (!temp_substr)
        return new_opt_str;

    xr_strcat(new_opt_str, new_opt_str_size, temp_substr);
    return new_opt_str;
}

#ifdef DEBUG
s32 lag_simmulator_min_ping = 0;
s32 lag_simmulator_max_ping = 0;
static bool SimmulateNetworkLag()
{
    static u32 max_lag_time = 0;

    if (!lag_simmulator_max_ping && !lag_simmulator_min_ping)
        return false;

    if (!max_lag_time || (max_lag_time <= Device.dwTimeGlobal))
    {
        CRandom tmp_random(Device.dwTimeGlobal);
        max_lag_time = Device.dwTimeGlobal + tmp_random.randI(lag_simmulator_min_ping, lag_simmulator_max_ping);
        return false;
    }
    return true;
}
#endif

void CLevel::ClientReceive()
{
    m_dwRPC = 0;
    m_dwRPS = 0;

    if (IsDemoPlayStarted())
    {
        SimulateServerUpdate();
    }
#ifdef DEBUG
    if (SimmulateNetworkLag())
        return;
#endif
    StartProcessQueue();
    for (NET_Packet* P = net_msg_Retreive(); P; P = net_msg_Retreive())
    {
        if (IsDemoSaveStarted())
        {
            SavePacket(*P);
        }
        //-----------------------------------------------------
        m_dwRPC++;
        m_dwRPS += P->B.count;
        //-----------------------------------------------------
        u16 m_type;
        u16 ID;
        P->r_begin(m_type);
        switch (m_type)
        {
        case M_SPAWN:
        {
            if (!bReady) //! m_bGameConfigStarted ||
            {
                Msg("! Unconventional M_SPAWN received : map_data[%s] | bReady[%s] | deny_m_spawn[%s]",
                    (map_data.m_map_sync_received) ? "true" : "false", (bReady) ? "true" : "false",
                    deny_m_spawn ? "true" : "false");
                break;
            }
            /*/
            cl_Process_Spawn(*P);
            /*/
            // Msg("--- Client received M_SPAWN message...");
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
            //*/
        }
        break;
        case M_EVENT:
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            // Msg("Client received M_EVENT message...");
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
            break;
        case M_EVENT_PACK:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            NET_Packet tmpP;
            while (!P->r_eof())
            {
                tmpP.B.count = P->r_u8();
                P->r(&tmpP.B.data, tmpP.B.count);
                tmpP.timeReceive = P->timeReceive;

                game_events->insert(tmpP);
                if (g_bDebugEvents)
                    ProcessGameEvents();
            };
        }
        break;
        case M_UPDATE: { game->net_import_update(*P);
        }
        break;
        case M_UPDATE_OBJECTS:
        {
            Objects.net_Import(P);

            if (OnClient())
                UpdateDeltaUpd(timeServer());
            IClientStatistic pStat = Level().GetStatistic();
            u32 dTime = 0;

            if ((Level().timeServer() + pStat.getPing()) < P->timeReceive)
            {
                dTime = pStat.getPing();
            }
            else
                dTime = Level().timeServer() - P->timeReceive + pStat.getPing();

            u32 NumSteps = physics_world()->CalcNumSteps(dTime);
            SetNumCrSteps(NumSteps);
        }
        break;
        case M_COMPRESSED_UPDATE_OBJECTS:
        {
            Flags8 compression_type;
            P->r_u8(compression_type.flags);
            ProcessCompressedUpdate(*P, compression_type);
        }
        break;
        case M_CL_UPDATE:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            if (OnClient())
                break;
            P->r_u16(ID);
            u32 Ping = P->r_u32();
            CGameObject* O = smart_cast<CGameObject*>(Objects.net_Find(ID));
            if (0 == O)
                break;
            O->net_Import(*P);
            //---------------------------------------------------
            UpdateDeltaUpd(timeServer());
            if (pObjects4CrPr.empty() && pActors4CrPr.empty())
                break;
            if (!smart_cast<CActor*>(O))
                break;

            u32 dTime = 0;
            if ((Level().timeServer() + Ping) < P->timeReceive)
            {
#ifdef DEBUG
//					Msg("! TimeServer[%d] < TimeReceive[%d]", Level().timeServer(), P->timeReceive);
#endif
                dTime = Ping;
            }
            else
                dTime = Level().timeServer() - P->timeReceive + Ping;
            u32 NumSteps = physics_world()->CalcNumSteps(dTime);
            SetNumCrSteps(NumSteps);

            O->CrPr_SetActivationStep(u32(physics_world()->StepsNum()) - NumSteps);
            AddActor_To_Actors4CrPr(O);
        }
        break;
        case M_MOVE_PLAYERS:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
        }
        break;
        // [08.11.07] Alexander Maniluk: added new message handler for moving artefacts.
        case M_MOVE_ARTEFACTS:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            u8 Count = P->r_u8();
            for (u8 i = 0; i < Count; ++i)
            {
                u16 ID = P->r_u16();
                Fvector NewPos;
                P->r_vec3(NewPos);
                CArtefact* OArtefact = smart_cast<CArtefact*>(Objects.net_Find(ID));
                if (!OArtefact)
                    break;
                OArtefact->MoveTo(NewPos);
                // destroy_physics_shell(OArtefact->PPhysicsShell());
            };
            /*NET_Packet PRespond;
            PRespond.w_begin(M_MOVE_ARTEFACTS_RESPOND);
            Send(PRespond, net_flags(TRUE, TRUE));*/
        }
        break;
        //------------------------------------------------
        case M_CL_INPUT:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            P->r_u16(ID);
            IGameObject* O = Objects.net_Find(ID);
            if (0 == O)
                break;
            O->net_ImportInput(*P);
        }
        break;
        //---------------------------------------------------
        case M_SV_CONFIG_NEW_CLIENT: InitializeClientGame(*P); break;
        case M_SV_CONFIG_GAME: game->net_import_state(*P); break;
        case M_SV_CONFIG_FINISHED: { game_configured = TRUE;
#ifdef DEBUG
            Msg("- Game configuring : Finished ");
#endif // #ifdef DEBUG
            if (IsDemoPlayStarted() && !m_current_spectator)
            {
                SpawnDemoSpectator();
            }
        }
        break;
        case M_MIGRATE_DEACTIVATE: // TO:   Changing server, just deactivate
        {
            NODEFAULT;
        }
        break;
        case M_MIGRATE_ACTIVATE: // TO:   Changing server, full state
        {
            NODEFAULT;
        }
        break;
        case M_CHAT:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            char buffer[256];
            P->r_stringZ(buffer);
            Msg("- %s", buffer);
        }
        break;
        case M_GAMEMESSAGE:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            if (!game)
                break;
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
        }
        break;
        case M_RELOAD_GAME:
        case M_LOAD_GAME:
        case M_CHANGE_LEVEL: {
#ifdef DEBUG
            Msg("--- Changing level message received...");
#endif // #ifdef DEBUG
            if (m_type == M_LOAD_GAME)
            {
                string256 saved_name;
                P->r_stringZ_s(saved_name);
                if (xr_strlen(saved_name) && ai().get_alife())
                {
                    CSavedGameWrapper wrapper(saved_name);
                    if (wrapper.level_id() == ai().level_graph().level_id())
                    {
                        Engine.Event.Defer("Game:QuickLoad", size_t(xr_strdup(saved_name)), 0);

                        break;
                    }
                }
            }
            MakeReconnect();
        }
        break;
        case M_SAVE_GAME: { ClientSave();
        }
        break;
        case M_GAMESPY_CDKEY_VALIDATION_CHALLENGE: { OnGameSpyChallenge(P);
        }
        break;
        case M_AUTH_CHALLENGE:
        {
            ClientSendProfileData();
            OnBuildVersionChallenge();
        }
        break;
        case M_CLIENT_CONNECT_RESULT: { OnConnectResult(P);
        }
        break;
        case M_CHAT_MESSAGE:
        {
            /*if (!game_configured)
            {
                Msg("! WARNING: ignoring game event [%d] - game not configured...", m_type);
                break;
            }*/
            if (!game)
                break;
            Game().OnChatMessage(P);
        }
        break;
        case M_CLIENT_WARN:
        {
            if (!game)
                break;
            Game().OnWarnMessage(P);
        }
        break;
        case M_REMOTE_CONTROL_AUTH:
        case M_REMOTE_CONTROL_CMD: { Game().OnRadminMessage(m_type, P);
        }
        break;
        case M_SV_MAP_NAME: { map_data.ReceiveServerMapSync(*P);
        }
        break;
        case M_SV_DIGEST: { SendClientDigestToServer();
        }
        break;
        case M_CHANGE_LEVEL_GAME:
        {
            Msg("- M_CHANGE_LEVEL_GAME Received");

            if (OnClient())
            {
                MakeReconnect();
            }
            else
            {
                const char* m_SO = m_caServerOptions.c_str();
                //					const char* m_CO = m_caClientOptions.c_str();

                m_SO = strchr(m_SO, '/');
                if (m_SO)
                    m_SO++;
                m_SO = strchr(m_SO, '/');

                shared_str LevelName;
                shared_str LevelVersion;
                shared_str GameType;

                P->r_stringZ(LevelName);
                P->r_stringZ(LevelVersion);
                P->r_stringZ(GameType);

                /*
                u32 str_start = P->r_tell();
                P->skip_stringZ();
                u32 str_end = P->r_tell();

                u32 temp_str_size = str_end - str_start;
                R_ASSERT2(temp_str_size < 256, "level name too big");
                LevelName = static_cast<char*>(xr_alloca(temp_str_size + 1));
                P->r_seek(str_start);
                P->r_stringZ(LevelName);


                str_start = P->r_tell();
                P->skip_stringZ();
                str_end = P->r_tell();
                temp_str_size = str_end - str_start;
                R_ASSERT2(temp_str_size < 256, "incorect game type");
                GameType = static_cast<char*>(xr_alloca(temp_str_size + 1));
                P->r_seek(str_start);
                P->r_stringZ(GameType);*/

                string4096 NewServerOptions = "";
                xr_sprintf(NewServerOptions, "%s/%s/%s%s", LevelName.c_str(), GameType.c_str(), map_ver_string,
                    LevelVersion.c_str());

                if (m_SO)
                {
                    string4096 additional_options;
                    xr_strcat(NewServerOptions, sizeof(NewServerOptions),
                        remove_version_option(m_SO, additional_options, sizeof(additional_options)));
                }
                m_caServerOptions = NewServerOptions;
                MakeReconnect();
            };
        }
        break;
        case M_CHANGE_SELF_NAME: { net_OnChangeSelfName(P);
        }
        break;
        case M_BULLET_CHECK_RESPOND:
        {
            if (!game)
                break;
            if (GameID() != eGameIDSingle)
                Game().m_WeaponUsageStatistic->On_Check_Respond(P);
        }
        break;
        case M_STATISTIC_UPDATE:
        {
            Msg("--- CL: On Update Request");
            if (!game)
                break;
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
        }
        break;
        case M_STATISTIC_UPDATE_RESPOND: // deprecated, see  xrServer::OnMessage
        {
            /*Msg("--- CL: On Update Respond");
            if (!game) break;
            if (GameID() != eGameIDSingle)
                Game().m_WeaponUsageStatistic->OnUpdateRespond(P);*/
        }
        break;
        case M_FILE_TRANSFER:
        {
            game_events->insert(*P);
            if (g_bDebugEvents)
                ProcessGameEvents();
        }
        break;
        case M_SECURE_KEY_SYNC: { OnSecureKeySync(*P);
        }
        break;
        case M_SECURE_MESSAGE: { OnSecureMessage(*P);
        }
        break;
        }

        net_msg_Release();
    }
    EndProcessQueue();

    if (g_bDebugEvents)
        ProcessGameSpawns();
}

void CLevel::OnMessage(void* data, u32 size) { IPureClient::OnMessage(data, size); };
