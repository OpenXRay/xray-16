#include "stdafx.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "xrserver.h"
#include "game_cl_base.h"
#include "game_sv_base.h"
#include "xrmessages.h"
#include "xrEngine/x_ray.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/xr_ioconsole.h"
#include "MainMenu.h"
#include "string_table.h"
#include "UIGameCustom.h"

int g_cl_save_demo = 0;

extern XRCORE_API bool g_allow_heap_min;

bool CLevel::net_Start(const char* op_server, const char* op_client)
{
    net_start_result_total = TRUE;

    pApp->LoadBegin();

	string64 player_name = "";
    xr_strcpy(player_name, xr_strlen(Core.UserName) ? Core.UserName : Core.CompName);
	VERIFY( xr_strlen(player_name) );

    string512 tmp;
    xr_strcpy(tmp, op_client);
    xr_strcat(tmp, "/name=");
    xr_strcat(tmp, player_name);
    m_caClientOptions = tmp;
    m_caServerOptions = op_server;
    //---------------------------------------------------------------------
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start1));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start2));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start4));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start6));

    return net_start_result_total;
}

bool CLevel::net_start1()
{
    // Start client and server if need it
    if (m_caServerOptions.size())
    {
        g_pGamePersistent->SetLoadStageTitle("st_server_starting");
        g_pGamePersistent->LoadTitle();

		typedef IGame_Persistent::params params;
		params &p = g_pGamePersistent->m_game_params;
		// Connect
		Server = new xrServer();

        if (xr_strcmp(p.m_alife, "alife"))
        {
            shared_str l_ver = game_sv_GameState::parse_level_version(m_caServerOptions);

            map_data.m_name = game_sv_GameState::parse_level_name(m_caServerOptions);

            if (!GEnv.isDedicatedServer)
                g_pGamePersistent->LoadTitle(true, map_data.m_name);

            int id = pApp->Level_ID(map_data.m_name.c_str(), l_ver.c_str(), true);

            if (id < 0)
            {
                Log("Can't find level: ", map_data.m_name.c_str());
                net_start_result_total = FALSE;
                return true;
            }
        }
    }
    else
    {
        g_allow_heap_min = false;
    }

    return true;
}

bool CLevel::net_start2()
{
    if (net_start_result_total && m_caServerOptions.size())
    {
        if ((m_connect_server_err = Server->Connect(m_caServerOptions)) != xrServer::ErrNoError)
        {
            net_start_result_total = false;
            Msg("! Failed to start server.");
            return true;
        }
        Server->SLS_Default();
        map_data.m_name = Server->level_name(m_caServerOptions);
        if (!GEnv.isDedicatedServer)
            g_pGamePersistent->LoadTitle(true, map_data.m_name);
    }
    return true;
}

bool CLevel::net_start4()
{
    if (!net_start_result_total)
        return true;

    g_loading_events.pop_front();

    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client6));
    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client5));
    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client4));
    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client3));
    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client2));
    g_loading_events.push_front(LOADING_EVENT(this, &CLevel::net_start_client1));

    return false;
}

bool CLevel::net_start6()
{
    // init bullet manager
    BulletManager().Clear();
    BulletManager().Load();

    pApp->LoadEnd();

    if (net_start_result_total)
    {
        if (strstr(Core.Params, "-$"))
        {
            string256 buf, cmd, param;
            sscanf(strstr(Core.Params, "-$") + 2, "%[^ ] %[^ ] ", cmd, param);
            strconcat(sizeof(buf), buf, cmd, " ", param);
            Console->Execute(buf);
        }
    }
    else
    {
        Msg("! Failed to start client. Check the connection or level existance.");

        if (!map_data.m_map_loaded && map_data.m_name.size())
        {
            LPCSTR level_id_string = NULL;
            LPCSTR dialog_string = NULL;
            LPCSTR download_url = !!map_data.m_map_download_url ? map_data.m_map_download_url.c_str() : "";
            LPCSTR tmp_map_ver = !!map_data.m_map_version ? map_data.m_map_version.c_str() : "";

            STRCONCAT(level_id_string, StringTable().translate("st_level"), ":", map_data.m_name.c_str(), "(", tmp_map_ver, "). ");
            STRCONCAT(dialog_string, level_id_string, StringTable().translate("ui_st_map_not_found"));

			DEL_INSTANCE(g_pGameLevel);
			Console->Execute("main_menu on");
		}
		else 
		{
			DEL_INSTANCE(g_pGameLevel);
			Console->Execute("main_menu on");
		}

        return true;
    }

    if (CurrentGameUI())
        CurrentGameUI()->OnConnected();

    return true;
}

void CLevel::InitializeClientGame(NET_Packet& P)
{
    if (game)
        return;

    xr_delete(game);
    game = new game_cl_Single();

    R_ASSERT(Load_GameSpecific_After());
}
