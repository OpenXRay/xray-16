#include "stdafx.h"

#include "Level.h"
#include "xrEngine/x_ray.h"
#include "xrEngine/IGame_Persistent.h"

#include "ai_space.h"
#include "game_cl_base.h"
#include "NET_Queue.h"
#include "hudmanager.h"

#include "xrPhysics/iphworld.h"

#include "phcommander.h"
#include "physics_game.h"
extern pureFrame* g_pNetProcessor;

bool CLevel::net_Start_client(const char* options) { return false; }
#include "string_table.h"
bool CLevel::net_start_client1()
{
    pApp->LoadBegin();

    // name_of_server
    string64 name_of_server = "";
    if (strchr(*m_caClientOptions, '/'))
        strncpy_s(name_of_server, *m_caClientOptions, strchr(*m_caClientOptions, '/') - *m_caClientOptions);

    if (strchr(name_of_server, '/'))
        *strchr(name_of_server, '/') = 0;

    string256 temp;
    xr_sprintf(temp, "%s %s",
               StringTable().translate("st_client_connecting_to").c_str(),
               name_of_server);

    pApp->SetLoadStageTitle(temp);
    pApp->LoadStage();
    return true;
}

#include "xrServer.h"

bool CLevel::net_start_client2()
{
    Server->CreateSVClient();

    // offline account creation
    ClientReceive();
    Server->Update();

    connected_to_server = Connect2Server(*m_caClientOptions);

    return true;
}

bool CLevel::net_start_client3()
{
    if (connected_to_server)
    {
        LPCSTR level_name = NULL;
        LPCSTR level_ver = NULL;
        LPCSTR download_url = NULL;

        shared_str const& server_options = Server->GetConnectOptions();
        level_name = name().c_str();
        level_ver = Server->level_version(server_options).c_str(); // 1.0

        // Determine internal level-ID
        int level_id = pApp->Level_ID(level_name, level_ver, true);
        if (level_id == -1)
        {
            connected_to_server = FALSE;
            Msg("! Level (name:%s), (version:%s), not found, try to download from:%s", level_name, level_ver,
                download_url);
            map_data.m_name = level_name;
            map_data.m_map_version = level_ver;
            map_data.m_map_download_url = download_url;
            map_data.m_map_loaded = false;
            return false;
        }
#ifdef DEBUG
        Msg("--- net_start_client3: level_id [%d], level_name[%s], level_version[%s]", level_id, level_name, level_ver);
#endif // #ifdef DEBUG
        map_data.m_name = level_name;
        map_data.m_map_version = level_ver;
        map_data.m_map_download_url = download_url;
        map_data.m_map_loaded = true;

        deny_m_spawn = FALSE;
        // Load level
        R_ASSERT2(Load(level_id), "Loading failed.");
    }
    return true;
}

bool CLevel::net_start_client4()
{
    if (connected_to_server)
    {
        // Begin spawn
        g_pGamePersistent->SetLoadStageTitle("st_client_spawning");
        g_pGamePersistent->LoadTitle();

        // Send physics to single or multithreaded mode

        create_physics_world(!!psDeviceFlags.test(mtPhysics), &ObjectSpace, &Objects, &Device);

        R_ASSERT(physics_world());

        m_ph_commander_physics_worldstep = new CPHCommander();
        physics_world()->set_update_callback(m_ph_commander_physics_worldstep);

        physics_world()->set_default_contact_shotmark(ContactShotMark);
        physics_world()->set_default_character_contact_shotmark(CharacterContactShotMark);

        VERIFY(physics_world());
        physics_world()->set_step_time_callback((PhysicsStepTimeCallback*)&PhisStepsCallback);

        // Send network to single or multithreaded mode
        // *note: release version always has "mt_*" enabled
        Device.seqFrameMT.Remove(g_pNetProcessor);
        Device.seqFrame.Remove(g_pNetProcessor);
        if (psDeviceFlags.test(mtNetwork))
            Device.seqFrameMT.Add(g_pNetProcessor, REG_PRIORITY_HIGH + 2);
        else
            Device.seqFrame.Add(g_pNetProcessor, REG_PRIORITY_LOW - 2);
    }
    return true;
}

bool CLevel::net_start_client5()
{
    if (connected_to_server)
    {
        // HUD

        // Textures
        if (!GEnv.isDedicatedServer)
        {
            g_pGamePersistent->SetLoadStageTitle("st_loading_textures");
            g_pGamePersistent->LoadTitle();
            GEnv.Render->DeferredLoad(FALSE);
            GEnv.Render->ResourcesDeferredUpload();
            LL_CheckTextures();
        }

        deny_m_spawn = TRUE;
    }
    return true;
}

bool CLevel::net_start_client6()
{
    if (connected_to_server)
    {
        // Sync
        if (!synchronize_map_data())
            return false;

        if (!game_configured)
        {
            pApp->LoadEnd();
            return true;
        }
        if (!GEnv.isDedicatedServer)
        {
            g_hud->Load();
            g_hud->OnConnected();
        }

#ifdef DEBUG
        Msg("--- net_start_client6");
#endif // #ifdef DEBUG

        if (game)
            game->OnConnected();

        g_pGamePersistent->SetLoadStageTitle("st_client_synchronising");
        pApp->LoadForceFinish();
        g_pGamePersistent->LoadTitle();
        Device.PreCache(60, true, true);
        net_start_result_total = TRUE;
    }
    else
    {
        net_start_result_total = FALSE;
    }

    pApp->LoadEnd();
    return true;
}
