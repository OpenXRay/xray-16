#include "StdAfx.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "xrServer.h"
#include "game_cl_base.h"
#include "xrMessages.h"
#include "xrGameSpyServer.h"
#include "xrEngine/x_ray.h"
#include "xrEngine/device.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/XR_IOConsole.h"
#include "MainMenu.h"
#include "string_table.h"
#include "UIGameCustom.h"
#include "ui/UICDkey.h"
#include "xrNetServer/NET_Messages.h"

int g_cl_save_demo = 0;

shared_str CLevel::OpenDemoFile(const char* demo_file_name)
{
    PrepareToPlayDemo(demo_file_name);
    return m_demo_server_options;
}
void CLevel::net_StartPlayDemo() { net_Start(m_demo_server_options.c_str(), "localhost"); }
bool CLevel::net_Start(const char* op_server, const char* op_client)
{
    net_start_result_total = TRUE;

    pApp->LoadBegin();

    string64 player_name;
    GetPlayerName_FromRegistry(player_name, sizeof(player_name));

    if (xr_strlen(player_name) == 0)
    {
        xr_strcpy(player_name, xr_strlen(Core.UserName) ? Core.UserName : Core.CompName);
    }
    VERIFY(xr_strlen(player_name));

    // make Client Name if options doesn't have it
    LPCSTR NameStart = strstr(op_client, "/name=");
    if (!NameStart)
    {
        string512 tmp;
        xr_strcpy(tmp, op_client);
        xr_strcat(tmp, "/name=");
        xr_strcat(tmp, player_name);
        m_caClientOptions = tmp;
    }
    else
    {
        string1024 ret = "";
        LPCSTR begin = NameStart + xr_strlen("/name=");
        sscanf(begin, "%[^/]", ret);
        if (!xr_strlen(ret))
        {
            string1024 tmpstr;
            xr_strcpy(tmpstr, op_client);
            *(strstr(tmpstr, "name=") + 5) = 0;
            xr_strcat(tmpstr, player_name);
            const char* ptmp = strstr(strstr(op_client, "name="), "/");
            if (ptmp)
                xr_strcat(tmpstr, ptmp);
            m_caClientOptions = tmpstr;
        }
        else
        {
            m_caClientOptions = op_client;
        };
    };
    m_caServerOptions = op_server;
    //---------------------------------------------------------------------
    if (!IsDemoPlay())
    {
        LPCSTR pdemosave = strstr(op_client, "/mpdemosave=");
        bool is_single = m_caServerOptions.size() != 0 ? (strstr(m_caServerOptions.c_str(), "single") != NULL) : false;
        int save_demo = g_cl_save_demo;
        if (pdemosave != NULL)
        {
            sscanf(pdemosave, "/mpdemosave=%d", &save_demo);
        }
        if (!is_single && save_demo)
        {
            PrepareToSaveDemo();
        }
    }
    //---------------------------------------------------------------------------
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start1));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start2));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start3));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start4));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start5));
    g_loading_events.push_back(LOADING_EVENT(this, &CLevel::net_start6));

    return net_start_result_total;
}

shared_str level_version(const shared_str& server_options);
shared_str level_name(const shared_str& server_options);
bool CLevel::net_start1()
{
    // Start client and server if need it
    if (m_caServerOptions.size())
    {
        g_pGamePersistent->SetLoadStageTitle("st_server_starting");
        g_pGamePersistent->LoadTitle();

        typedef IGame_Persistent::params params;
        params& p = g_pGamePersistent->m_game_params;
        // Connect
        if (!xr_strcmp(p.m_game_type, "single"))
        {
            Server = new xrServer();
        }
        else
        {
            Server = new xrGameSpyServer();
        }

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

    return true;
}

bool CLevel::net_start2()
{
    if (net_start_result_total && m_caServerOptions.size())
    {
        GameDescriptionData game_descr;
        if ((m_connect_server_err = Server->Connect(m_caServerOptions, game_descr)) != xrServer::ErrNoError)
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

bool CLevel::net_start3()
{
    if (!net_start_result_total)
        return true;
    // add server port if don't have one in options
    if (!strstr(m_caClientOptions.c_str(), "port=") && Server)
    {
        string64 PortStr;
        xr_sprintf(PortStr, "/port=%d", Server->GetPort());

        string4096 tmp;
        xr_strcpy(tmp, m_caClientOptions.c_str());
        xr_strcat(tmp, PortStr);

        m_caClientOptions = tmp;
    }
    // add password string to client, if don't have one
    if (m_caServerOptions.size())
    {
        if (strstr(m_caServerOptions.c_str(), "psw=") && !strstr(m_caClientOptions.c_str(), "psw="))
        {
            string64 PasswordStr = "";
            const char* PSW = strstr(m_caServerOptions.c_str(), "psw=") + 4;
            if (strchr(PSW, '/'))
                strncpy_s(PasswordStr, PSW, strchr(PSW, '/') - PSW);
            else
                xr_strcpy(PasswordStr, PSW);

            string4096 tmp;
            xr_sprintf(tmp, "%s/psw=%s", m_caClientOptions.c_str(), PasswordStr);
            m_caClientOptions = tmp;
        };
    };
    // setting players GameSpy CDKey if it comes from command line
    if (strstr(m_caClientOptions.c_str(), "/cdkey="))
    {
        string64 CDKey;
        const char* start = strstr(m_caClientOptions.c_str(), "/cdkey=") + xr_strlen("/cdkey=");
        sscanf(start, "%[^/]", CDKey);
        string128 cmd;
        xr_sprintf(cmd, "cdkey %s", xr_strupr(CDKey));
        Console->Execute(cmd);
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

bool CLevel::net_start5()
{
    if (net_start_result_total)
    {
        NET_Packet NP;
        NP.w_begin(M_CLIENTREADY);
        Game().local_player->net_Export(NP, TRUE);
        Send(NP, net_flags(TRUE, TRUE));

        if (OnClient() && Server)
        {
            Server->SLS_Clear();
        };
    };
    return true;
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

        if (m_connect_server_err == xrServer::ErrConnect && !psNET_direct_connect && !GEnv.isDedicatedServer)
        {
            DEL_INSTANCE(g_pGameLevel);
            Console->Execute("main_menu on");

            MainMenu()->SwitchToMultiplayerMenu();
        }
        else if (!map_data.m_map_loaded && map_data.m_name.size() &&
            m_bConnectResult) // if (map_data.m_name == "") - level not loaded, see CLevel::net_start_client3
        {
            LPCSTR level_id_string = NULL;
            LPCSTR dialog_string = NULL;
            LPCSTR download_url = !!map_data.m_map_download_url ? map_data.m_map_download_url.c_str() : "";
            LPCSTR tmp_map_ver = !!map_data.m_map_version ? map_data.m_map_version.c_str() : "";

            STRCONCAT(level_id_string, StringTable().translate("st_level"), ":", map_data.m_name.c_str(), "(", tmp_map_ver, "). ");
            STRCONCAT(dialog_string, level_id_string, StringTable().translate("ui_st_map_not_found"));

            DEL_INSTANCE(g_pGameLevel);
            Console->Execute("main_menu on");

            if (!GEnv.isDedicatedServer)
            {
                MainMenu()->SwitchToMultiplayerMenu();
                MainMenu()->Show_DownloadMPMap(dialog_string, download_url);
            }
        }
        else if (map_data.IsInvalidClientChecksum())
        {
            LPCSTR level_id_string = NULL;
            LPCSTR dialog_string = NULL;
            LPCSTR download_url = !!map_data.m_map_download_url ? map_data.m_map_download_url.c_str() : "";
            LPCSTR tmp_map_ver = !!map_data.m_map_version ? map_data.m_map_version.c_str() : "";

            STRCONCAT(level_id_string, StringTable().translate("st_level"), ":", map_data.m_name.c_str(), "(", tmp_map_ver, "). ");
            STRCONCAT(dialog_string, level_id_string, StringTable().translate("ui_st_map_data_corrupted"));

            g_pGameLevel->net_Stop();
            DEL_INSTANCE(g_pGameLevel);
            Console->Execute("main_menu on");
            if (!GEnv.isDedicatedServer)
            {
                MainMenu()->SwitchToMultiplayerMenu();
                MainMenu()->Show_DownloadMPMap(dialog_string, download_url);
            }
        }
        else
        {
            DEL_INSTANCE(g_pGameLevel);
            Console->Execute("main_menu on");
        }

        return true;
    }

    if (!GEnv.isDedicatedServer)
    {
        if (CurrentGameUI())
            CurrentGameUI()->OnConnected();
    }

    return true;
}

void CLevel::InitializeClientGame(NET_Packet& P)
{
    string256 game_type_name;
    P.r_stringZ(game_type_name);
    if (game && !xr_strcmp(game_type_name, game->type_name()))
        return;

    xr_delete(game);
#ifdef DEBUG
    Msg("- Game configuring : Started ");
#endif // #ifdef DEBUG
    CLASS_ID clsid = game_GameState::getCLASS_ID(game_type_name, false);
    game = smart_cast<game_cl_GameState*>(NEW_INSTANCE(clsid));
    game->set_type_name(game_type_name);
    game->Init();
    m_bGameConfigStarted = TRUE;

    if (!IsGameTypeSingle())
    {
        init_compression();
    }

    R_ASSERT(Load_GameSpecific_After());
}
