#include "StdAfx.h"
#include "xrMessages.h"
#include "xrGameSpyServer.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "xrGameSpy/xrGameSpy.h"
#include "xrGameSpy/GameSpy_Available.h"
#include "xrGameSpy/GameSpy_GCD_Server.h"

#define STRING_KICKED_BY_SERVER "st_kicked_by_server"
u32 g_sv_max_suspicious_actions = 5;
u32 g_sv_suspicious_actions_ban_time = 30; // minutes

//#define DEMO_BUILD

xrGameSpyServer::xrGameSpyServer()
{
    m_iReportToMasterServer = 0;
    m_bQR2_Initialized = FALSE;
    m_bCDKey_Initialized = FALSE;
    m_bCheckCDKey = false;
    ServerFlags.set(server_flag_all, 0);
    iGameSpyBasePort = 0;
}

xrGameSpyServer::~xrGameSpyServer()
{
    CDKey_ShutDown();
    QR2_ShutDown();
}

bool xrGameSpyServer::HasPassword() { return !!ServerFlags.test(server_flag_password); }
bool xrGameSpyServer::HasProtected() { return !!ServerFlags.test(server_flag_protected); }
//----------- xrGameSpyClientData -----------------------
IClient* xrGameSpyServer::client_Create() { return new xrGameSpyClientData(); }
xrGameSpyClientData::xrGameSpyClientData() : xrClientData()
{
    m_bCDKeyAuth = false;
    m_iCDKeyReauthHint = 0;
    suspiciousActionCount = 0;
}
void xrGameSpyClientData::Clear()
{
    inherited::Clear();

    m_pChallengeString[0] = 0;
    m_bCDKeyAuth = false;
    m_iCDKeyReauthHint = 0;
    suspiciousActionCount = 0;
};

xrGameSpyClientData::~xrGameSpyClientData()
{
    m_pChallengeString[0] = 0;
    m_bCDKeyAuth = false;
    m_iCDKeyReauthHint = 0;
    suspiciousActionCount = 0;
}
//-------------------------------------------------------
xrGameSpyServer::EConnect xrGameSpyServer::Connect(shared_str& session_name, GameDescriptionData& game_descr)
{
    EConnect res = inherited::Connect(session_name, game_descr);
    if (res != ErrNoError)
        return res;

    if (0 == *(game->get_option_s(*session_name, "hname", NULL)))
    {
        string1024 CompName;
        DWORD CompNameSize = 1024;
#ifndef LINUX // FIXME!!!
        if (GetComputerName(CompName, &CompNameSize))
            HostName = CompName;
#endif
    }
    else
        HostName = game->get_option_s(*session_name, "hname", NULL);

    if (0 != *(game->get_option_s(*session_name, "psw", NULL)))
        Password._set(game->get_option_s(*session_name, "psw", NULL));

    string4096 tMapName = "";
    const char* SName = *session_name;
    strncpy_s(tMapName, *session_name, strchr(SName, '/') - SName);
    MapName = tMapName; // = (session_name);

    m_iReportToMasterServer = game->get_option_i(*session_name, "public", 0);
    m_iMaxPlayers = game->get_option_i(*session_name, "maxplayers", 32);
    //	m_bCheckCDKey = game->get_option_i		(*session_name,"cdkey",0) != 0;
    m_bCheckCDKey = game->get_option_i(*session_name, "public", 0) != 0;
    //--------------------------------------------//
    if (game->Type() != eGameIDSingle)
    {
        //----- Check for Backend Services ---
        CGameSpy_Available GSA;
        shared_str result_string;
        if (!GSA.CheckAvailableServices(result_string))
        {
            Msg(*result_string);
        };

        //------ Init of QR2 SDK -------------
        iGameSpyBasePort = game->get_option_i(*session_name, "portgs", -1);
        QR2_Init(iGameSpyBasePort);

//------ Init of CDKey SDK -----------

#ifndef DEBUG

#ifndef DEMO_BUILD
        if (m_bCheckCDKey)
#endif

            CDKey_Init();
#endif // DEBUG
    };

    return res;
}

void xrGameSpyServer::Update()
{
    inherited::Update();

    if (m_bQR2_Initialized)
    {
        m_QR2.Think(NULL);
    };

    if (m_bCDKey_Initialized)
    {
        m_GCDServer.Think();
    };
}

int xrGameSpyServer::GetPlayersCount()
{
    int NumPlayers = net_players.ClientsCount();
    if (!GEnv.isDedicatedServer || NumPlayers < 1)
        return NumPlayers;
    return NumPlayers - 1;
};

bool xrGameSpyServer::NeedToCheckClient_GameSpy_CDKey(IClient* CL)
{
    if (!m_bCDKey_Initialized || (CL == GetServerClient() && GEnv.isDedicatedServer))
    {
        return false;
    };

    SendChallengeString_2_Client(CL);

    return true;
};

void xrGameSpyServer::OnCL_Disconnected(IClient* _CL)
{
    inherited::OnCL_Disconnected(_CL);

    if (m_bCDKey_Initialized)
    {
        Msg("Server : Disconnecting Client");
        m_GCDServer.DisconnectUser(int(_CL->ID.value()));
    };
}

u32 xrGameSpyServer::OnMessage(NET_Packet& P, ClientID sender) // Non-Zero means broadcasting with "flags" as returned
{
    u16 type;
    P.r_begin(type);

    xrGameSpyClientData* CL = (xrGameSpyClientData*)ID_to_client(sender);

    switch (type)
    {
    case M_GAMESPY_CDKEY_VALIDATION_CHALLENGE_RESPOND:
    {
        string128 ResponseStr = "";
        u32 bytesRemain = P.r_elapsed();
        if (bytesRemain == 0 || bytesRemain > sizeof(ResponseStr))
        {
            xr_string clientIp = CL->m_cAddress.to_string();
            Msg("! WARNING: Validation challenge respond from client [%s] is %s. DoS attack?", clientIp.c_str(),
                bytesRemain == 0 ? "empty" : "too long");

            CL->suspiciousActionCount++;

            if (CL->suspiciousActionCount > g_sv_max_suspicious_actions)
                BanClient(CL, g_sv_suspicious_actions_ban_time);

            DisconnectClient(CL, STRING_KICKED_BY_SERVER);

            return 0;
        }
        P.r_stringZ(ResponseStr);
        if (!CL->m_bCDKeyAuth)
        {
#ifndef MASTER_GOLD
            Msg("Server : Respond accepted, Authenticate client.");
#endif // #ifndef MASTER_GOLD
            CGameSpy_GCD_Server::ClientAuthCallback authCb;
            authCb.bind(this, &xrGameSpyServer::OnCDKey_Validation);
            CGameSpy_GCD_Server::ClientReauthCallback reauthCb;
            reauthCb.bind(this, &xrGameSpyServer::OnCDKey_ReValidation);
            m_GCDServer.AuthUser(
                int(CL->ID.value()), CL->m_cAddress.m_data.data, CL->m_pChallengeString, ResponseStr, authCb, reauthCb);
            xr_strcpy(CL->m_guid, 128, this->GCD_Server()->GetKeyHash(CL->ID.value()));
        }
        else
        {
            Msg("Server : Respond accepted, ReAuthenticate client.");
            m_GCDServer.ReAuthUser(int(CL->ID.value()), CL->m_iCDKeyReauthHint, ResponseStr);
        }

        return (0);
    }
    break;
    }

    return inherited::OnMessage(P, sender);
};

bool xrGameSpyServer::Check_ServerAccess(IClient* CL, string512& reason)
{
    if (!HasProtected())
    {
        xr_strcpy(reason, "Access successful by server. ");
        return true;
    }
    // CL->pass to check the server password
    return true;
}

void xrGameSpyServer::Assign_ServerType(string512& res)
{
    string_path fn;
    FS.update_path(fn, "$app_data_root$", "server_users.ltx");
    if (FS.exist(fn))
    {
        CInifile inif(fn);
        if (inif.section_exist("users"))
        {
            if (inif.line_count("users") != 0)
            {
                ServerFlags.set(server_flag_protected, 1);
                xr_strcpy(res, "# Server started as protected, using users list.");
                Msg(res);
                return;
            }
            else
            {
                xr_strcpy(res, "Users count in list is null.");
            }
        }
        else
        {
            xr_strcpy(res, "Section [users] not found.");
        }
    }
    else
    {
        xr_strcpy(res, "File <server_users.ltx> not found in folder <$app_data_root$>.");
    } // if FS.exist(fn)

    Msg(res);
    ServerFlags.set(server_flag_protected, 0);
    xr_strcpy(res, "# Server started without users list.");
    Msg(res);
}

void xrGameSpyServer::GetServerInfo(CServerInfo* si)
{
    string32 tmp, tmp2;

    si->AddItem("Server name", HostName.c_str(), RGB(128, 128, 255));
    si->AddItem("Map", MapName.c_str(), RGB(255, 0, 128));

    xr_strcpy(tmp, xr_itoa(GetPlayersCount(), tmp2, 10));
    xr_strcat(tmp, " / ");
    xr_strcat(tmp, xr_itoa(m_iMaxPlayers, tmp2, 10));
    si->AddItem("Players", tmp, RGB(255, 128, 255));

    string256 res;
    si->AddItem("Game version", GetGameVersion(), RGB(0, 158, 255));

    xr_strcpy(res, "");
    if (HasProtected() || (Password.size() > 0))
    {
        if (HasProtected())
            xr_strcat(res, "protected  ");
        if (Password.size() > 0)
            xr_strcat(res, "password  ");
    }
    else
    {
        if (xr_strlen(res) == 0)
            xr_strcat(res, "free");
    }
    si->AddItem("Access to server", res, RGB(200, 155, 155));

    si->AddItem("GameSpy port", xr_itoa(iGameSpyBasePort, tmp, 10), RGB(200, 5, 155));
    inherited::GetServerInfo(si);
}
