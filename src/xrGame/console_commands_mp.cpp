#include "StdAfx.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "Level.h"
#include "xrServer.h"
#include "game_cl_base.h"
#include "game_cl_mp.h"
#include "Actor.h"
#include "xrServer_Object_Base.h"
#include "RegistryFuncs.h"
#include "GamePersistent.h"
#include "MainMenu.h"
#include "UIGameCustom.h"
#include "game_sv_deathmatch.h"
#include "game_sv_artefacthunt.h"
#include "game_sv_capture_the_artefact.h"
#include "date_time.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "string_table.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "DemoPlay_Control.h"
#include "account_manager_console.h"
#include "xrGameSpy/GameSpy_GP.h"
#include "xrNetServer/NET_Messages.h"

EGameIDs ParseStringToGameType(LPCSTR str);
LPCSTR GameTypeToString(EGameIDs gt, bool bShort);
LPCSTR AddHyphens(LPCSTR c);
LPCSTR DelHyphens(LPCSTR c);

extern float g_cl_lvInterp;
extern int g_cl_InterpolationType; // 0 - Linear, 1 - BSpline, 2 - HSpline
extern u32 g_cl_InterpolationMaxPoints;
extern int g_cl_save_demo;
extern string64 gsCDKey;
extern u32 g_dwMaxCorpses;
extern float g_fTimeFactor;
extern BOOL g_b_COD_PickUpMode;
extern int g_iWeaponRemove;
extern int g_iCorpseRemove;
extern BOOL g_bCollectStatisticData;
// extern	BOOL	g_bStatisticSaveAuto	;
extern BOOL g_SV_Disable_Auth_Check;
extern BOOL g_sv_ignore_version_mismatch;

extern int g_sv_mp_iDumpStatsPeriod;
extern BOOL g_SV_Force_Artefact_Spawn;
extern int g_Dump_Update_Write;
extern int g_Dump_Update_Read;
extern u32 g_sv_base_dwRPointFreezeTime;
extern int g_sv_base_iVotingEnabled;
extern BOOL g_sv_mp_bSpectator_FreeFly;
extern BOOL g_sv_mp_bSpectator_FirstEye;
extern BOOL g_sv_mp_bSpectator_LookAt;
extern BOOL g_sv_mp_bSpectator_FreeLook;
extern BOOL g_sv_mp_bSpectator_TeamCamera;
extern BOOL g_sv_mp_bCountParticipants;
extern float g_sv_mp_fVoteQuota;
extern float g_sv_mp_fVoteTime;
extern u32 g_sv_dm_dwForceRespawn;
extern s32 g_sv_dm_dwFragLimit;
extern s32 g_sv_dm_dwTimeLimit;
extern BOOL g_sv_dm_bDamageBlockIndicators;
extern u32 g_sv_dm_dwDamageBlockTime;
extern BOOL g_sv_dm_bAnomaliesEnabled;
extern u32 g_sv_dm_dwAnomalySetLengthTime;
extern BOOL g_sv_dm_bPDAHunt;
extern u32 g_sv_dm_dwWarmUp_MaxTime;
extern BOOL g_sv_dm_bDMIgnore_Money_OnBuy;
extern BOOL g_sv_tdm_bAutoTeamBalance;
extern BOOL g_sv_tdm_bAutoTeamSwap;
extern BOOL g_sv_tdm_bFriendlyIndicators;
extern BOOL g_sv_tdm_bFriendlyNames;
extern float g_sv_tdm_fFriendlyFireModifier;
extern int g_sv_tdm_iTeamKillLimit;
extern int g_sv_tdm_bTeamKillPunishment;
extern u32 g_sv_ah_dwArtefactRespawnDelta;
extern int g_sv_ah_dwArtefactsNum;
extern u32 g_sv_ah_dwArtefactStayTime;
extern int g_sv_ah_iReinforcementTime;
extern BOOL g_sv_ah_bBearerCantSprint;
extern BOOL g_sv_ah_bShildedBases;
extern BOOL g_sv_ah_bAfReturnPlayersToBases;
extern u32 g_sv_dwMaxClientPing;
extern int g_be_message_out;

extern int g_sv_Skip_Winner_Waiting;
extern int g_sv_Wait_For_Players_Ready;
extern int G_DELAYED_ROUND_TIME;
extern int g_sv_Pending_Wait_Time;
extern u32 g_sv_Client_Reconnect_Time;
int g_dwEventDelay = 0;

extern u32 g_sv_max_suspicious_actions;
extern u32 g_sv_suspicious_actions_ban_time;

extern u32 g_sv_adm_menu_ban_time;
extern const xr_token g_ban_times[];

extern int g_sv_adm_menu_ping_limit;
extern u32 g_sv_cta_dwInvincibleTime;
// extern	u32		g_sv_cta_dwAnomalySetLengthTime;
extern u32 g_sv_cta_artefactReturningTime;
extern u32 g_sv_cta_activatedArtefactRet;
// extern	s32		g_sv_cta_ScoreLimit;
extern u32 g_sv_cta_PlayerScoresDelayTime;
extern u32 g_sv_cta_rankUpToArtsCountDiv;

extern BOOL g_draw_downloads;
extern BOOL g_sv_mp_save_proxy_screenshots;
extern BOOL g_sv_mp_save_proxy_configs;

#ifdef DEBUG
extern s32 lag_simmulator_min_ping;
extern s32 lag_simmulator_max_ping;
#endif

extern BOOL g_sv_write_updates_bin;
extern Flags8 g_sv_traffic_optimization_level;
extern Flags8 g_sv_available_traffic_optimization_level;

void XRNETSERVER_API DumpNetCompressorStats(bool brief);
BOOL XRNETSERVER_API g_net_compressor_enabled;
BOOL XRNETSERVER_API g_net_compressor_gather_stats;

class CCC_Restart : public IConsole_Command
{
public:
    CCC_Restart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server)
        {
            Level().Server->GetGameState()->SetRoundResult(eRoundEnd_GameRestarted);
            Level().Server->GetGameState()->OnRoundEnd();
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "restart game"); }
};

class CCC_RestartFast : public IConsole_Command
{
public:
    CCC_RestartFast(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server)
        {
            Level().Server->GetGameState()->SetRoundResult(eRoundEnd_GameRestartedFast);
            Level().Server->GetGameState()->OnRoundEnd();
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "restart game fast"); }
};

class CCC_Kill : public IConsole_Command
{
public:
    CCC_Kill(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (IsGameTypeSingle())
            return;
        if (!g_pGameLevel)
            return;
        if (Game().local_player && Game().local_player->testFlag(GAME_PLAYER_FLAG_VERY_VERY_DEAD))
            return;

        IGameObject* l_pObj = Level().CurrentControlEntity();
        CActor* l_pPlayer = smart_cast<CActor*>(l_pObj);
        if (l_pPlayer)
        {
            NET_Packet P;
            l_pPlayer->u_EventGen(P, GE_GAME_EVENT, l_pPlayer->ID());
            P.w_u16(GAME_EVENT_PLAYER_KILL);
            P.w_u16(u16(l_pPlayer->ID()));
            l_pPlayer->u_EventSend(P);
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "player kill"); }
};

class CCC_Net_CL_Resync : public IConsole_Command
{
public:
    CCC_Net_CL_Resync(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args) { Level().net_Syncronize(); }
    virtual void Info(TInfo& I) { xr_strcpy(I, "resyncronize client"); }
};

class CCC_Net_CL_ClearStats : public IConsole_Command
{
public:
    CCC_Net_CL_ClearStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args) { Level().ClearStatistic(); }
    virtual void Info(TInfo& I) { xr_strcpy(I, "clear client net statistic"); }
};

class CCC_Net_SV_ClearStats : public IConsole_Command
{
public:
    CCC_Net_SV_ClearStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args) { Level().Server->ClearStatistic(); }
    virtual void Info(TInfo& I) { xr_strcpy(I, "clear server net statistic"); }
};

#ifdef DEBUG
class CCC_Dbg_NumObjects : public IConsole_Command
{
public:
    CCC_Dbg_NumObjects(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        u32 SVObjNum = (OnServer()) ? Level().Server->GetEntitiesNum() : 0;
        xr_vector<u16> SObjID;
        for (u32 i = 0; i < SVObjNum; i++)
        {
            CSE_Abstract* pEntity = Level().Server->GetEntity(i);
            SObjID.push_back(pEntity->ID);
        };
        std::sort(SObjID.begin(), SObjID.end());

        u32 CLObjNum = Level().Objects.o_count();
        xr_vector<u16> CObjID;
        for (u32 i = 0; i < CLObjNum; i++)
        {
            CObjID.push_back(Level().Objects.o_get_by_iterator(i)->ID());
        };
        std::sort(CObjID.begin(), CObjID.end());

        Msg("Client Objects : %d", CLObjNum);
        Msg("Server Objects : %d", SVObjNum);

        for (u32 CO = 0; CO < _max(CLObjNum, SVObjNum); CO++)
        {
            if (CO < CLObjNum && CO < SVObjNum)
            {
                CSE_Abstract* pEntity = Level().Server->ID_to_entity(SObjID[CO]);
                IGameObject* pObj = Level().Objects.net_Find(CObjID[CO]);
                char color = (pObj->ID() == pEntity->ID) ? '-' : '!';

                Msg("%c%4d: Client - %20s[%5d] <===> Server - %s [%d]", color, CO + 1, *(pObj->cNameSect()), pObj->ID(),
                    pEntity->s_name.c_str(), pEntity->ID);
            }
            else
            {
                if (CO < CLObjNum)
                {
                    IGameObject* pObj = Level().Objects.net_Find(CObjID[CO]);
                    Msg("! %2d: Client - %s [%d] <===> Server - -----------------", CO + 1, *(pObj->cNameSect()),
                        pObj->ID());
                }
                else
                {
                    CSE_Abstract* pEntity = Level().Server->ID_to_entity(SObjID[CO]);
                    Msg("! %2d: Client - ----- <===> Server - %s [%d]", CO + 1, pEntity->s_name.c_str(), pEntity->ID);
                }
            }
        };

        Msg("Client Objects : %d", CLObjNum);
        Msg("Server Objects : %d", SVObjNum);
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "dbg Num Objects"); }
};
#endif // DEBUG

extern void WriteCDKey_ToRegistry(pstr cdkey);

class CCC_GSCDKey : public CCC_String
{
public:
    CCC_GSCDKey(LPCSTR N, pstr V, int _size) : CCC_String(N, V, _size) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR arguments)
    {
        string64 cdkey;
        if (0 == xr_stricmp(arguments, "clear"))
        {
            cdkey[0] = 0;
        }
        else
        {
            xr_strcpy(cdkey, arguments);
        }

        u32 cdkey_len = xr_strlen(cdkey);
        if ((cdkey_len > 0) && g_pGamePersistent && MainMenu())
        {
            if ((cdkey_len > 5) && cdkey[4] != '-')
            {
                LPCSTR res = AddHyphens(cdkey);
                xr_strcpy(cdkey, res);
            }

            CCC_String::Execute(cdkey);
            WriteCDKey_ToRegistry(cdkey);

            if (!MainMenu()->ValidateCDKey())
            {
#ifdef DEBUG
                Msg("! Invalid CD-Key");
#endif // DEBUG
            }
        }
    } // Execute
    virtual void Save(IWriter* F){};

    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        tips.push_back("xxxx-xxxx-xxxx-xxxx");
        tips.push_back("clear");
    }
};

// most useful predicates
struct SearcherClientByName
{
    string512 player_name;
    SearcherClientByName(LPCSTR name)
    {
        strncpy_s(player_name, sizeof(player_name), name, sizeof(player_name) - 1);
        xr_strlwr(player_name);
    }
    bool operator()(IClient* client)
    {
        xrClientData* temp_client = smart_cast<xrClientData*>(client);
        pstr tmp_player = NULL;
        if (!temp_client->ps)
            return false;

        STRCONCAT(tmp_player, temp_client->ps->getName());
        xr_strlwr(tmp_player);

        if (!xr_strcmp(player_name, tmp_player))
        {
            return true;
        }
        return false;
    }
};

#define STRING_KICKED_BY_SERVER "st_kicked_by_server"

class CCC_KickPlayerByName : public IConsole_Command
{
public:
    CCC_KickPlayerByName(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        if (!xr_strlen(args))
            return;
        if (strchr(args, '/'))
        {
            Msg("!  '/' is not allowed in names!");
            return;
        }
        string4096 PlayerName = "";
        u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
        if (xr_strlen(args) > max_name_length)
        {
            strncpy_s(PlayerName, args, max_name_length);
            PlayerName[max_name_length] = 0;
        }
        else
            xr_strcpy(PlayerName, args);

        xr_strlwr(PlayerName);
        IClient* tmp_client = Level().Server->FindClient(SearcherClientByName(PlayerName));
        if (tmp_client && (tmp_client != Level().Server->GetServerClient()))
        {
            Msg("Disconnecting : %s", PlayerName);
            xrClientData* tmpxrclient = static_cast<xrClientData*>(tmp_client);
            if (!tmpxrclient->m_admin_rights.m_has_admin_rights)
            {
                Level().Server->DisconnectClient(tmp_client, STRING_KICKED_BY_SERVER);
            }
            else
            {
                Msg("! Can't disconnect client with admin rights");
            }
        }
        else
        {
            Msg("! Can't disconnect player [%s]", PlayerName);
        }
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Kick Player by name"); }
};

static ClientID last_printed_player;
#define LAST_PRINTED_PLAYER_STR "last_printed"
static u32 last_printed_player_banned;
#define LAST_PRINTED_PLAYER_BANNED_STR "last_printed_banned"

class CCC_KickPlayerByID : public IConsole_Command
{
public:
    CCC_KickPlayerByID(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;

        u32 len = xr_strlen(args);
        if ((len == 0) || (len >= 128)) // one digit and raid:%u
            return;
        ClientID client_id(0);

        if (!strncmp(args, LAST_PRINTED_PLAYER_STR, sizeof(LAST_PRINTED_PLAYER_STR) - 1))
        {
            client_id = last_printed_player;
        }
        else
        {
            u32 tmp_client_id;
            if (sscanf(args, "%u", &tmp_client_id) != 1)
            {
                Msg("! ERROR: bad command parameters.");
                Msg("Kick player. Format: \"sv_kick_id <player session id | \'%s\'>\". To receive list of players ids "
                    "see "
                    "sv_listplayers",
                    LAST_PRINTED_PLAYER_STR);
                return;
            }
            client_id.set(tmp_client_id);
        }

        IClient* tmp_client = Level().Server->GetClientByID(client_id);

        if (tmp_client && (tmp_client != Level().Server->GetServerClient()))
        {
            Msg("Disconnecting : client %u", client_id.value());
            xrClientData* tmpxrclient = static_cast<xrClientData*>(tmp_client);
            if (!tmpxrclient->m_admin_rights.m_has_admin_rights)
            {
                Level().Server->DisconnectClient(tmp_client, STRING_KICKED_BY_SERVER);
            }
            else
            {
                Msg("! Can't disconnect client with admin rights %u", client_id.value());
            }
        }
        else
        {
            Msg("! Can't disconnect client %u", client_id.value());
        }
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Kick player by ID."); }
};

#define RAPREFIX "raid:"
static xrClientData* exclude_command_initiator(LPCSTR args)
{
    LPCSTR tmp_str = strrchr(args, ' ');
    if (!tmp_str)
        tmp_str = args;
    LPCSTR clientidstr = strstr(tmp_str, RAPREFIX);
    if (clientidstr)
    {
        clientidstr += sizeof(RAPREFIX) - 1;
        u32 client_id = static_cast<u32>(strtoul(clientidstr, NULL, 10));
        ClientID tmp_id;
        tmp_id.set(client_id);
        if (g_pGameLevel && Level().Server)
            return Level().Server->ID_to_client(tmp_id);
    }
    return NULL;
};
static char const* exclude_raid_from_args(LPCSTR args, pstr dest, size_t dest_size)
{
    strncpy_s(dest, dest_size, args, dest_size - 1);
    char* tmp_str = strrchr(dest, ' ');
    if (!tmp_str)
        tmp_str = dest;
    char* raidstr = strstr(tmp_str, RAPREFIX);
    if (raidstr)
    {
        if (raidstr <= tmp_str)
            *raidstr = 0;
        else
            *(raidstr - 1) = 0; // with ' '
    }
    dest[dest_size - 1] = 0;
    return dest;
}

class CCC_MakeScreenshot : public IConsole_Command
{
public:
    CCC_MakeScreenshot(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        u32 len = xr_strlen(args_);
        if ((len == 0) || (len >= 256)) // two digits and raid:%u
            return;

        ClientID client_id = 0;
        if (!strncmp(args_, LAST_PRINTED_PLAYER_STR, sizeof(LAST_PRINTED_PLAYER_STR) - 1))
        {
            client_id = last_printed_player;
        }
        else
        {
            u32 tmp_client_id;
            if (sscanf(args_, "%u", &tmp_client_id) != 1)
            {
                Msg("! ERROR: bad command parameters.");
                Msg("Make screenshot. Format: \"make_screenshot <player session id | \'%s\'> <ban_time_in_sec>\". To "
                    "receive list of players ids see sv_listplayers",
                    LAST_PRINTED_PLAYER_STR);
                return;
            }
            client_id.set(tmp_client_id);
        }
        xrClientData* admin_client = exclude_command_initiator(args_);
        if (!admin_client)
        {
            Msg("! ERROR: only radmin can make screenshots ...");
            return;
        }
        Level().Server->MakeScreenshot(admin_client->ID, client_id);
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, make_string("Make screenshot. Format: \"make_screenshot <player session id | \'%s\'> "
                                 "<ban_time_in_sec>\". To receive list of players ids see sv_listplayers",
                         LAST_PRINTED_PLAYER_STR)
                         .c_str());
    }

}; // class CCC_MakeScreenshot

class CCC_MakeConfigDump : public IConsole_Command
{
public:
    CCC_MakeConfigDump(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        u32 len = xr_strlen(args_);
        if ((len == 0) || (len >= 256)) // two digits and raid:%u
            return;

        ClientID client_id = 0;
        if (!strncmp(args_, LAST_PRINTED_PLAYER_STR, sizeof(LAST_PRINTED_PLAYER_STR) - 1))
        {
            client_id = last_printed_player;
        }
        else
        {
            u32 tmp_client_id;
            if (sscanf(args_, "%u", &tmp_client_id) != 1)
            {
                Msg("! ERROR: bad command parameters.");
                Msg("Make screenshot. Format: \"make_config_dump <player session id | \'%s\'> <ban_time_in_sec>\". To "
                    "receive list of players ids see sv_listplayers",
                    LAST_PRINTED_PLAYER_STR);
                return;
            }
            client_id.set(tmp_client_id);
        }
        xrClientData* admin_client = exclude_command_initiator(args_);
        if (!admin_client)
        {
            Msg("! ERROR: only radmin can make config dumps ...");
            return;
        }
        Level().Server->MakeConfigDump(admin_client->ID, client_id);
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, make_string("Make config dump. Format: \"make_config_dump <player session id | \'%s\'> "
                                 "<ban_time_in_sec>\". To receive list of players ids see sv_listplayers",
                         LAST_PRINTED_PLAYER_STR)
                         .c_str());
    }

}; // class CCC_MakeConfigDump

class CCC_SetDemoPlaySpeed : public IConsole_Command
{
public:
    CCC_SetDemoPlaySpeed(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        float new_speed;
        sscanf(args, "%f", &new_speed);
        Level().SetDemoPlaySpeed(new_speed);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Set demo play speed (0.0, 8.0]"); }
}; // class CCC_SetDemoPlaySpeed

class DemoPlayControlArgParser
{
protected:
    demoplay_control::EAction m_action;
    shared_str m_action_param;
    bool ParseControlString(LPCSTR args_string)
    {
        string16 action_name;
        action_name[0] = 0;
        string32 param_name;
        param_name[0] = 0;

        sscanf(args_string, "%16s %32s", action_name, param_name);
        m_action_param = param_name;

        if (!xr_strcmp(action_name, "roundstart"))
        {
            m_action = demoplay_control::on_round_start;
        }
        else if (!xr_strcmp(action_name, "kill"))
        {
            m_action = demoplay_control::on_kill;
        }
        else if (!xr_strcmp(action_name, "die"))
        {
            m_action = demoplay_control::on_die;
        }
        else if (!xr_strcmp(action_name, "artefacttake"))
        {
            m_action = demoplay_control::on_artefactcapturing;
        }
        else if (!xr_strcmp(action_name, "artefactdrop"))
        {
            m_action = demoplay_control::on_artefactloosing;
        }
        else if (!xr_strcmp(action_name, "artefactdeliver"))
        {
            m_action = demoplay_control::on_artefactdelivering;
        }
        else
        {
            return false;
        }
        return true;
    };
    inline LPCSTR GetInfoString()
    {
        return "<roundstart,kill,die,artefacttake,artefactdrop,artefactdeliver> [player name]";
    }
}; // class DemoPlayControlArgParser

class CCC_DemoPlayPauseOn : public IConsole_Command, public DemoPlayControlArgParser
{
public:
    CCC_DemoPlayPauseOn(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        if (!ParseControlString(args))
        {
            TInfo tmp_info;
            Info(tmp_info);
            Msg(tmp_info);
            return;
        }
        demoplay_control* dp_control = Level().GetDemoPlayControl();
        R_ASSERT(dp_control);
        dp_control->pause_on(m_action, m_action_param);
    };

    virtual void Info(TInfo& I)
    {
        LPCSTR info_str = NULL;
        STRCONCAT(info_str, "Play demo until specified event (then pause playing). Format: mpdemoplay_pause_on ",
            DemoPlayControlArgParser::GetInfoString());
        xr_strcpy(I, info_str);
    }
}; // class CCC_DemoPlayPauseOn

class CCC_DemoPlayCancelPauseOn : public IConsole_Command
{
public:
    CCC_DemoPlayCancelPauseOn(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        demoplay_control* dp_control = Level().GetDemoPlayControl();
        R_ASSERT(dp_control);
        dp_control->cancel_pause_on();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Cancels mpdemoplay_pause_on."); }
}; // class CCC_DemoPlayCancelPauseOn

class CCC_DemoPlayRewindUntil : public IConsole_Command, public DemoPlayControlArgParser
{
public:
    CCC_DemoPlayRewindUntil(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        if (!ParseControlString(args))
        {
            TInfo tmp_info;
            Info(tmp_info);
            Msg(tmp_info);
            return;
        }
        demoplay_control* dp_control = Level().GetDemoPlayControl();
        R_ASSERT(dp_control);
        dp_control->rewind_until(m_action, m_action_param);
    };

    virtual void Info(TInfo& I)
    {
        LPCSTR info_str = NULL;
        STRCONCAT(info_str, "Rewind demo until specified event (then pause playing). Format: mpdemoplay_rewind_until ",
            DemoPlayControlArgParser::GetInfoString());
        xr_strcpy(I, info_str);
    }
}; // class CCC_DemoPlayRewindUntil

class CCC_DemoPlayStopRewind : public IConsole_Command
{
public:
    CCC_DemoPlayStopRewind(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        demoplay_control* dp_control = Level().GetDemoPlayControl();
        R_ASSERT(dp_control);
        dp_control->stop_rewind();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Stops rewinding (mpdemoplay_rewind_until)."); }
}; // class CCC_DemoPlayStopRewind

class CCC_DemoPlayRestart : public IConsole_Command
{
public:
    CCC_DemoPlayRestart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlay())
        {
            Msg("! No demo play started.");
            return;
        }
        Level().RestartPlayDemo();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Restarts playing demo."); }
}; // class CCC_DemoPlayRestart

class CCC_MulDemoPlaySpeed : public IConsole_Command
{
public:
    CCC_MulDemoPlaySpeed(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        Level().SetDemoPlaySpeed(Level().GetDemoPlaySpeed() * 2);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Increases demo play speed"); };
};

class CCC_DivDemoPlaySpeed : public IConsole_Command
{
public:
    CCC_DivDemoPlaySpeed(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().IsDemoPlayStarted())
        {
            Msg("! Demo play not started.");
            return;
        }
        float curr_demo_speed = Level().GetDemoPlaySpeed();
        if (curr_demo_speed <= 0.2f)
        {
            Msg("! Can't decrease demo speed");
            return;
        }
        Level().SetDemoPlaySpeed(curr_demo_speed / 2);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Decreases demo play speed"); };
};

class CCC_ScreenshotAllPlayers : public IConsole_Command
{
public:
    CCC_ScreenshotAllPlayers(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server)
            return;
        struct ScreenshotMaker
        {
            xrClientData* admin_client;
            void operator()(IClient* C) { Level().Server->MakeScreenshot(admin_client->ID, C->ID); }
        };
        ScreenshotMaker tmp_functor;
        tmp_functor.admin_client = exclude_command_initiator(args_);
        if (!tmp_functor.admin_client)
        {
            Msg("! ERROR: only radmin can make screenshots (use \"ra login\")");
            return;
        }
        Level().Server->ForEachClientDo(tmp_functor);
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, "Make screenshot of each player in the game. Format: \"screenshot_all");
    }

}; // class CCC_ScreenshotAllPlayers

class CCC_ConfigsDumpAll : public IConsole_Command
{
public:
    CCC_ConfigsDumpAll(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server)
            return;
        struct ConfigDumper
        {
            xrClientData* admin_client;
            void operator()(IClient* C) { Level().Server->MakeConfigDump(admin_client->ID, C->ID); }
        };
        ConfigDumper tmp_functor;
        tmp_functor.admin_client = exclude_command_initiator(args_);
        if (!tmp_functor.admin_client)
        {
            Msg("! ERROR: only radmin can make config dumps (use \"ra login\")");
            return;
        }
        Level().Server->ForEachClientDo(tmp_functor);
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, "Make config dump of each player in the game. Format: \"config_dump_all");
    }
}; // class CCC_ConfigsDumpAll

#ifdef DEBUG

class CCC_DbgMakeScreenshot : public IConsole_Command
{
public:
    CCC_DbgMakeScreenshot(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server)
            return;
        ClientID server_id(Level().Server->GetServerClient()->ID);
        Level().Server->MakeScreenshot(server_id, server_id);
    }
}; // CCC_DbgMakeScreenshot

#endif //#ifdef DEBUG

class CCC_BanPlayerByCDKEY : public IConsole_Command
{
public:
    CCC_BanPlayerByCDKEY(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        game_sv_mp* tmp_sv_game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
        if (!tmp_sv_game)
            return;

        u32 len = xr_strlen(args_);
        if ((len == 0) || (len >= 256)) // two digits and raid:%u
            return;

        ClientID client_id = 0;
        s32 ban_time = 0;
        if (!strncmp(args_, LAST_PRINTED_PLAYER_STR, sizeof(LAST_PRINTED_PLAYER_STR) - 1))
        {
            client_id = last_printed_player;
            if (sscanf(args_ + sizeof(LAST_PRINTED_PLAYER_STR), "%d", &ban_time) != 1)
            {
                Msg("! ERROR: bad command parameters.");
                Msg("Ban player. Format: \"sv_banplayer <player session id | \'%s\'> <ban_time_in_sec>\". To receive "
                    "list "
                    "of players ids see sv_listplayers",
                    LAST_PRINTED_PLAYER_STR);
                return;
            }
        }
        else
        {
            u32 tmp_client_id;
            if (sscanf(args_, "%u %d", &tmp_client_id, &ban_time) != 2)
            {
                Msg("! ERROR: bad command parameters.");
                Msg("Ban player. Format: \"sv_banplayer <player session id | \'%s\'> <ban_time_in_sec>\". To receive "
                    "list "
                    "of players ids see sv_listplayers",
                    LAST_PRINTED_PLAYER_STR);
                return;
            }
            client_id.set(tmp_client_id);
        }

        IClient* to_disconnect = tmp_sv_game->BanPlayer(client_id, ban_time, exclude_command_initiator(args_));
        if (to_disconnect)
        {
            Level().Server->DisconnectClient(to_disconnect, STRING_KICKED_BY_SERVER);
        }
        else
        {
            Msg("! ERROR: bad client id [%u]", client_id.value());
        }
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, make_string("Ban player. Format: \"sv_banplayer <player session id | \'%s\'> <ban_time_in_sec>\". "
                                 "To receive list of players ids see sv_listplayers",
                         LAST_PRINTED_PLAYER_STR)
                         .c_str());
    }
};

class CCC_BanPlayerByCDKEYDirectly : public IConsole_Command
{
public:
    CCC_BanPlayerByCDKEYDirectly(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        game_sv_mp* tmp_sv_game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
        if (!tmp_sv_game)
            return;

        u32 len = xr_strlen(args_);
        if ((len == 0) || (len >= 256))
            return;

        char hex_digest[64];
        s32 ban_time = 0;
        if (sscanf(args_, "%s %i", &hex_digest, &ban_time) != 2)
        {
            Msg("! ERROR: bad command parameters.");
            Msg("Ban player. Format: \"sv_banplayer_by_digest <hex digest> <ban_time_in_sec>\". To get player hex "
                "digest "
                "you can enter: sv_listplayers_banned");
            return;
        }

        tmp_sv_game->BanPlayerDirectly(hex_digest, ban_time, exclude_command_initiator(args_));
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I,
            "Ban player by hex digest (CAREFULLY: low level command). Format: \"sv_banplayer_by_digest <hex digest> "
            "<ban_time_in_sec>\". To get player hex digest you can enter: sv_listplayers_banned");
    }
};

class CCC_UnBanPlayerByIndex : public IConsole_Command
{
public:
    CCC_UnBanPlayerByIndex(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        game_sv_mp* tmp_sv_game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
        if (!tmp_sv_game)
            return;
        u32 len = xr_strlen(args_);
        if ((len == 0) || (len >= 64)) // one digit and raid:%u
            return;

        if (!strncmp(args_, LAST_PRINTED_PLAYER_BANNED_STR, sizeof(LAST_PRINTED_PLAYER_BANNED_STR) - 1))
        {
            tmp_sv_game->UnBanPlayer(last_printed_player_banned);
        }
        else
        {
            // XXX: size_t ????? u32 maybe?
            size_t player_index = 0;
            if (sscanf(args_, "%zu", &player_index) != 1)
            {
                Msg("! ERROR: bad command parameters.");
                Msg(" Unban player. Format: \"sv_unbanplayer <banned player index | \'%s\'>. To receive list of banned "
                    "players se sv_listplayers_banned",
                    LAST_PRINTED_PLAYER_BANNED_STR);
                return;
            }
            tmp_sv_game->UnBanPlayer(player_index);
        }
    }
    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, make_string("Unban player. Format: \"sv_unbanplayer <banned player index | \'%s\'>. To receive "
                                 "list of banned players see sv_listplayers_banned",
                         LAST_PRINTED_PLAYER_BANNED_STR)
                         .c_str());
    }
};

class CCC_BanPlayerByName : public IConsole_Command
{
public:
    CCC_BanPlayerByName(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        string4096 buff;
        xr_strcpy(buff, args_);
        u32 len = xr_strlen(buff);

        if (0 == len)
            return;

        string1024 digits;
        pstr p = buff + len - 1;
        while (isdigit(*p))
        {
            if (p == buff)
                break;
            --p;
        }
        R_ASSERT(p >= buff);
        xr_strcpy(digits, p);
        *p = 0;
        if (!xr_strlen(buff))
        {
            Msg("incorrect parameter passed. bad name.");
            return;
        }

        u32 ban_time = atol(digits);
        if (ban_time == 0)
        {
            Msg("incorrect parameters passed.  name and time required");
            return;
        }
        string4096 PlayerName = "";
        u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
        if (xr_strlen(buff) > max_name_length)
        {
            strncpy_s(PlayerName, buff, max_name_length);
            PlayerName[max_name_length] = 0;
        }
        else
            xr_strcpy(PlayerName, buff);

        xr_strlwr(PlayerName);

        IClient* tmp_client = Level().Server->FindClient(SearcherClientByName(PlayerName));
        if (tmp_client && (tmp_client != Level().Server->GetServerClient()))
        {
            Msg("Disconnecting and Banning: %s", PlayerName);
            Level().Server->BanClient(tmp_client, ban_time);
            Level().Server->DisconnectClient(tmp_client, STRING_KICKED_BY_SERVER);
        }
        else
        {
            Msg("! Can't disconnect player [%s]", PlayerName);
        }
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Ban Player by Name"); }
};

class CCC_BanPlayerByIP : public IConsole_Command
{
public:
    CCC_BanPlayerByIP(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args_)
    {
        if (!g_pGameLevel || !Level().Server)
            return;
        //-----------
        string4096 buff;
        exclude_raid_from_args(args_, buff, sizeof(buff)); // xr_strcpy(buff, args_);

        u32 len = xr_strlen(buff);
        if (0 == len)
            return;

        string1024 digits;
        pstr p = buff + len - 1;
        while (isdigit(*p))
        {
            if (p == buff)
                break;
            --p;
        }
        R_ASSERT(p >= buff);
        xr_strcpy(digits, p);
        *p = 0;
        if (!xr_strlen(buff))
        {
            Msg("incorrect parameter passed. bad IP address.");
            return;
        }
        u32 ban_time = atol(digits);
        if (ban_time == 0)
        {
            Msg("incorrect parameters passed.  IP and time required");
            return;
        }

        string1024 s_ip_addr;
        xr_strcpy(s_ip_addr, buff);
        //-----------

        ip_address Address;
        Address.set(s_ip_addr);
        Msg("Disconnecting and Banning: %s", Address.to_string().c_str());
        Level().Server->BanAddress(Address, ban_time);
        Level().Server->DisconnectAddress(Address, STRING_KICKED_BY_SERVER);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Ban Player by IP. Format: \"sb_banplayer_ip <ip address>\""); }
};

class CCC_UnBanPlayerByIP : public IConsole_Command
{
public:
    CCC_UnBanPlayerByIP(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server)
            return;

        if (!xr_strlen(args))
            return;

        string4096 buff;
        exclude_raid_from_args(args, buff, sizeof(buff)); // xr_strcpy(buff, args_);
        if (!xr_strlen(buff))
            return;

        ip_address Address;
        Address.set(buff);
        Level().Server->UnBanAddress(Address);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "UnBan Player by IP. Format: \"sv_unbanplayer_ip <ip address>\""); }
};

class CCC_ListPlayers : public IConsole_Command
{
public:
    CCC_ListPlayers(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;

        u32 cnt = Level().Server->GetGameState()->get_players_count();
        Msg("- Total Players : %d", cnt);
        Msg("- ----player list begin-----");
        struct PlayersEnumerator
        {
            LPCSTR filter_string;
            PlayersEnumerator() { filter_string = NULL; }
            void operator()(IClient* client)
            {
                xrClientData* l_pC = (xrClientData*)client;
                if (!l_pC)
                    return;
                ip_address Address;
                DWORD dwPort = 0;
                Level().Server->GetClientAddress(client->ID, Address, &dwPort);
                string512 tmp_string;
                xr_sprintf(tmp_string, "- (player session id : %u), (name : %s), (ip: %s), (ping: %u);",
                    client->ID.value(), l_pC->ps->getName(), Address.to_string().c_str(), l_pC->ps->ping);
                if (filter_string)
                {
                    if (strstr(tmp_string, filter_string))
                    {
                        Msg(tmp_string);
                        last_printed_player = client->ID;
                    }
                }
                else
                {
                    Msg(tmp_string);
                    last_printed_player = client->ID;
                }
            }
        };
        PlayersEnumerator tmp_functor;
        string512 filter_string;
        string512 tmp_dest;
        if (xr_strlen(args))
        {
            exclude_raid_from_args(args, tmp_dest, sizeof(tmp_dest));
            if (xr_strlen(tmp_dest))
            {
                sscanf(tmp_dest, "%s", filter_string);
                tmp_functor.filter_string = filter_string;
            }
        }
        Level().Server->ForEachClientDo(tmp_functor);
        Msg("- ----player list end-------");
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "List Players. Format: \"sv_listplayers [ filter string ]\""); }
};

class CCC_Name : public IConsole_Command
{
public:
    CCC_Name(LPCSTR N) : IConsole_Command(N)
    {
        bLowerCaseArgs = false;
        bEmptyArgsHandled = false;
    };
    virtual void GetStatus(TStatus& S)
    {
        S[0] = 0;
        if (IsGameTypeSingle())
            return;
        if (!(&Level()))
            return;
        if (!(&Game()))
            return;
        game_PlayerState* tmp_player = Game().local_player;
        if (!tmp_player)
            return;
        xr_sprintf(S, "is \"%s\" ", tmp_player->getName());
    }

    virtual void Save(IWriter* F) {}
    virtual void Execute(LPCSTR args)
    {
        if (IsGameTypeSingle())
            return;
        if (!(&Level()))
            return;
        if (!(&Game()))
            return;

        game_PlayerState* tmp_player = Game().local_player;
        if (!tmp_player)
            return;

        if (tmp_player->m_account.is_online())
        {
            Msg("! Can't change name in online mode.");
            return;
        }

        if (!xr_strlen(args))
            return;
        if (strchr(args, '/'))
        {
            Msg("!  '/' is not allowed in names!");
            return;
        }
        string4096 NewName = "";
        u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
        if (xr_strlen(args) > max_name_length)
        {
            strncpy_s(NewName, args, max_name_length);
            NewName[max_name_length] = 0;
        }
        else
            xr_strcpy(NewName, args);

        NET_Packet P;
        Game().u_EventGen(P, GE_GAME_EVENT, Game().local_player->GameID);
        P.w_u16(GAME_EVENT_PLAYER_NAME);
        P.w_stringZ(NewName);
        Game().u_EventSend(P);
    }

    virtual void Info(TInfo& I) { xr_strcpy(I, "player name"); }
};

class CCC_ListPlayers_Banned : public IConsole_Command
{
public:
    CCC_ListPlayers_Banned(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        game_sv_mp* tmp_sv_game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
        if (!tmp_sv_game)
            return;
        string512 tmp_dest;
        string512 filter_dest = "";
        exclude_raid_from_args(args, tmp_dest, sizeof(tmp_dest));
        if (xr_strlen(tmp_dest))
        {
            sscanf(tmp_dest, "%s", filter_dest);
        }
        tmp_sv_game->PrintBanList(filter_dest);
        Level().Server->Print_Banned_Addreses();
    };

    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, "List of Banned Players. Format: \"sv_listplayers_banned [ filter string ]\"");
    }
};

class CCC_ChangeLevelGameType : public IConsole_Command
{
public:
    CCC_ChangeLevelGameType(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (!xr_strlen(args))
        {
            Msg("Changing level, version and game type. Arguments: <level name> <level version> <game type>");
            return;
        }

        string256 LevelName;
        LevelName[0] = 0;
        string256 LevelVersion;
        LevelVersion[0] = 0;
        string256 GameType;
        GameType[0] = 0;

        sscanf(args, "%255s %255s %255s", LevelName, LevelVersion, GameType);

        EGameIDs GameTypeID = ParseStringToGameType(GameType);
        if (GameTypeID == eGameIDNoGame)
        {
            Msg("! Unknown gametype - %s", GameType);
            return;
        };
        //-----------------------------------------

        const SGameTypeMaps& M = gMapListHelper.GetMapListFor(GameTypeID);
        u32 cnt = M.m_map_names.size();
        bool bMapFound = false;
        for (u32 i = 0; i < cnt; ++i)
        {
            const MPLevelDesc& itm = M.m_map_names[i];
            if (!xr_strcmp(itm.map_name.c_str(), LevelName) && !xr_strcmp(itm.map_ver.c_str(), LevelVersion))
            {
                bMapFound = true;
                break;
            }
        }
        if (!bMapFound)
        {
            Msg("! Level [%s][%s] not found for [%s]!", LevelName, LevelVersion, GameType);
            return;
        }

        NET_Packet P;
        P.w_begin(M_CHANGE_LEVEL_GAME);
        P.w_stringZ(LevelName);
        P.w_stringZ(LevelVersion);
        P.w_stringZ(GameType);
        Level().Send(P);
    };

    virtual void Info(TInfo& I)
    {
        xr_strcpy(I, "Changing level, version and game type. Arguments: <level name> <level version> <game type>");
    }
};

class CCC_ChangeGameType : public CCC_ChangeLevelGameType
{
public:
    CCC_ChangeGameType(LPCSTR N) : CCC_ChangeLevelGameType(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        // string256			GameType;
        // GameType[0]			=0;
        // sscanf				(args,"%s", GameType);

        string1024 argsNew;
        xr_sprintf(argsNew, "%s %s %s", Level().name().c_str(), Level().version().c_str(), args);

        CCC_ChangeLevelGameType::Execute((LPCSTR)argsNew);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Changing Game Type : <dm>,<tdm>,<ah>,<cta>"); }
    virtual void fill_tips(vecTips& tips, u32 mode)
    {
        if (g_pGameLevel && Level().Server && OnServer() && Level().Server->GetGameState())
        {
            EGameIDs type = Level().Server->GetGameState()->Type();
            TStatus str;
            xr_sprintf(str, sizeof(str), "%s  (current game type)  [dm,tdm,ah,cta]", GameTypeToString(type, true));
            tips.push_back(str);
        }
        IConsole_Command::fill_tips(tips, mode);
    }
};

class CCC_ChangeLevel : public CCC_ChangeLevelGameType
{
public:
    CCC_ChangeLevel(LPCSTR N) : CCC_ChangeLevelGameType(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (!xr_strlen(args))
        {
            Msg("Changing Game Type. Arguments: <level name> <level version>");
            return;
        }

        string256 LevelName;
        string256 LevelVersion;
        LevelName[0] = 0;
        LevelVersion[0] = 0;
        sscanf(args, "%255s %255s", LevelName, LevelVersion);

        string1024 argsNew;
        xr_sprintf(argsNew, "%s %s %s", LevelName, LevelVersion, Level().Server->GetGameState()->type_name());

        CCC_ChangeLevelGameType::Execute((LPCSTR)argsNew);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Changing Game Type. Arguments: <level name> <level version>"); }
};

class CCC_AddMap : public IConsole_Command
{
public:
    CCC_AddMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;

        string512 MapName, MapVer;
        LPCSTR c = strstr(args, "/ver=");
        if (!c)
            strncpy_s(MapName, sizeof(MapName), args, sizeof(MapName) - 1);
        else
        {
            strncpy_s(MapName, sizeof(MapName), args, c - args);
            xr_strcpy(MapVer, sizeof(MapVer), c + 5);
        }

        Level().Server->GetGameState()->MapRotation_AddMap(MapName, MapVer);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Adds map to map rotation list"); }
};

class CCC_ListMaps : public IConsole_Command
{
public:
    CCC_ListMaps(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        Level().Server->GetGameState()->MapRotation_ListMaps();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "List maps in map rotation list"); }
};

class CCC_NextMap : public IConsole_Command
{
public:
    CCC_NextMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        Level().Server->GetGameState()->OnNextMap();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Switch to Next Map in map rotation list"); }
};

class CCC_PrevMap : public IConsole_Command
{
public:
    CCC_PrevMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        Level().Server->GetGameState()->OnPrevMap();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Switch to Previous Map in map rotation list"); }
};

class CCC_AnomalySet : public IConsole_Command
{
public:
    CCC_AnomalySet(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        game_sv_Deathmatch* gameDM = smart_cast<game_sv_Deathmatch*>(Level().Server->GetGameState());
        if (!gameDM)
            return;

        gameDM->StartAnomalies(atol(args));
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Activating pointed Anomaly set"); }
};

class CCC_Vote_Start : public IConsole_Command
{
public:
    CCC_Vote_Start(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (IsGameTypeSingle())
        {
            Msg("! Only for multiplayer games!");
            return;
        }

        if (!Game().IsVotingEnabled())
        {
            Msg("! Voting is disabled by server!");
            return;
        }
        if (Game().IsVotingActive())
        {
            Msg("! There is voting already!");
            return;
        }

        u16 game_phase = Game().Phase();
        if ((game_phase != GAME_PHASE_INPROGRESS) && (game_phase != GAME_PHASE_PENDING))
        {
            Msg("! Voting is allowed only when game is in progress!");
            return;
        };

        Game().SendStartVoteMessage(args);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Starts Voting"); };
};

class CCC_Vote_Stop : public IConsole_Command
{
public:
    CCC_Vote_Stop(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        if (IsGameTypeSingle())
        {
            Msg("! Only for multiplayer games!");
            return;
        }

        if (!Level().Server->GetGameState()->IsVotingEnabled())
        {
            Msg("! Voting is disabled by server!");
            return;
        }

        if (!Level().Server->GetGameState()->IsVotingActive())
        {
            Msg("! Currently there is no active voting!");
            return;
        }

        if (Level().Server->GetGameState()->Phase() != GAME_PHASE_INPROGRESS)
        {
            Msg("! Voting is allowed only when game is in progress!");
            return;
        };

        Level().Server->GetGameState()->OnVoteStop();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Stops Current Voting"); };
};

class CCC_Vote_Yes : public IConsole_Command
{
public:
    CCC_Vote_Yes(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (IsGameTypeSingle())
        {
            Msg("! Only for multiplayer games!");
            return;
        }

        if (!Game().IsVotingEnabled())
        {
            Msg("! Voting is disabled by server!");
            return;
        }

        if (!Game().IsVotingActive())
        {
            Msg("! Currently there is no active voting!");
            return;
        }

        if (Game().Phase() != GAME_PHASE_INPROGRESS)
        {
            Msg("! Voting is allowed only when game is in progress!");
            return;
        };

        Game().SendVoteYesMessage();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Vote Yes"); };
};

class CCC_Vote_No : public IConsole_Command
{
public:
    CCC_Vote_No(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (IsGameTypeSingle())
        {
            Msg("! Only for multiplayer games!");
            return;
        }

        if (!Game().IsVotingEnabled())
        {
            Msg("! Voting is disabled by server!");
            return;
        }

        if (!Game().IsVotingActive())
        {
            Msg("! Currently there is no active voting!");
            return;
        }

        if (Game().Phase() != GAME_PHASE_INPROGRESS)
        {
            Msg("! Voting is allowed only when game is in progress!");
            return;
        };

        Game().SendVoteNoMessage();
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Vote No"); };
};

class CCC_StartTimeEnvironment : public IConsole_Command
{
public:
    CCC_StartTimeEnvironment(LPCSTR N) : IConsole_Command(N){};
    virtual void Execute(LPCSTR args)
    {
        u32 hours = 0, mins = 0;

        sscanf(args, "%d:%d", &hours, &mins);
        u64 NewTime = generate_time(1, 1, 1, hours, mins, 0, 0);

        if (!g_pGameLevel)
            return;

        if (!Level().Server)
            return;

        if (!Level().Server->GetGameState())
            return;

        float eFactor = Level().Server->GetGameState()->GetEnvironmentGameTimeFactor();
        Level().Server->GetGameState()->SetEnvironmentGameTimeFactor(NewTime, eFactor);
        Level().Server->GetGameState()->SetGameTimeFactor(NewTime, g_fTimeFactor);
    }
};
class CCC_SetWeather : public IConsole_Command
{
public:
    CCC_SetWeather(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR weather_name)
    {
        if (!g_pGamePersistent)
            return;
        if (!OnServer())
            return;

        if (weather_name && weather_name[0])
        {
            g_pGamePersistent->Environment().SetWeather(weather_name);
        }
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Set new weather"); }
};

class CCC_SaveStatistic : public IConsole_Command
{
public:
    CCC_SaveStatistic(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!Level().Server)
            return;
        game_sv_mp* sv_game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
        if (!sv_game)
        {
            Msg("! Server multiplayer game instance not present");
            return;
        }
        sv_game->DumpRoundStatistics();
        // Game().m_WeaponUsageStatistic->SaveData();
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "saving statistic data"); }
};

class CCC_AuthCheck : public CCC_Integer
{
public:
    CCC_AuthCheck(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};
    virtual void Save(IWriter* F){};
};

class CCC_ReturnToBase : public IConsole_Command
{
public:
    CCC_ReturnToBase(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (GameID() != eGameIDArtefactHunt)
            return;

        game_sv_ArtefactHunt* g = smart_cast<game_sv_ArtefactHunt*>(Level().Server->GetGameState());
        g->MoveAllAlivePlayers();
    }
};

class CCC_GetServerAddress : public IConsole_Command
{
public:
    CCC_GetServerAddress(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        ip_address Address;
        u32 dwPort = 0;

        Level().GetServerAddress(Address, &dwPort);

        Msg("Server Address - %s:%i", Address.to_string().c_str(), dwPort);
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "List Players"); }
};

class CCC_StartTeamMoney : public IConsole_Command
{
public:
    CCC_StartTeamMoney(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;

        game_sv_mp* pGameMP = smart_cast<game_sv_Deathmatch*>(Level().Server->GetGameState());
        if (!pGameMP)
            return;

        string512 Team = "";
        s32 TeamMoney = 0;
        sscanf(args, "%s %i", Team, &TeamMoney);

        if (!Team[0])
        {
            Msg("- --------------------");
            Msg("Teams start money:");
            u32 TeamCount = pGameMP->GetTeamCount();
            for (u32 i = 0; i < TeamCount; i++)
            {
                TeamStruct* pTS = pGameMP->GetTeamData(i);
                if (!pTS)
                    continue;
                Msg("Team %d: %d", i, pTS->m_iM_Start);
            }
            Msg("- --------------------");
            return;
        }
        else
        {
            u32 TeamID = 0;
            s32 TeamStartMoney = 0;
            sscanf(args, "%i %i", &TeamID, &TeamStartMoney);
            TeamStruct* pTS = pGameMP->GetTeamData(TeamID);
            if (pTS)
                pTS->m_iM_Start = TeamStartMoney;
        }
    };

    virtual void Info(TInfo& I) { xr_strcpy(I, "Set Start Team Money"); }
};
class CCC_SV_Integer : public CCC_Integer
{
public:
    CCC_SV_Integer(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max){};

    virtual void Execute(LPCSTR args)
    {
        CCC_Integer::Execute(args);

        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;

        Level().Server->GetGameState()->signal_Syncronize();
    }
};

class CCC_SV_Float : public CCC_Float
{
public:
    CCC_SV_Float(LPCSTR N, float* V, float _min = 0, float _max = 1) : CCC_Float(N, V, _min, _max){};

    virtual void Execute(LPCSTR args)
    {
        CCC_Float::Execute(args);
        if (!g_pGameLevel || !Level().Server || !Level().Server->GetGameState())
            return;
        Level().Server->GetGameState()->signal_Syncronize();
    }
};
class CCC_RadminCmd : public IConsole_Command
{
public:
    CCC_RadminCmd(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR arguments)
    {
        if (IsGameTypeSingle() || xr_strlen(arguments) >= 512)
        {
            return;
        }

        if (strstr(arguments, "login") == arguments)
        {
            string512 user;
            string512 pass;
            if (2 == sscanf(arguments + xr_strlen("login") + 1, "%s %s", user, pass))
            {
                NET_Packet P;
                P.w_begin(M_REMOTE_CONTROL_AUTH);
                P.w_stringZ(user);
                P.w_stringZ(pass);

                Level().Send(P, net_flags(TRUE, TRUE));
            }
            else
                Msg("2 args(user pass) needed");
        }
        else if (strstr(arguments, "logout") == arguments)
        {
            NET_Packet P;
            P.w_begin(M_REMOTE_CONTROL_AUTH);
            P.w_stringZ("logoff");

            Level().Send(P, net_flags(TRUE, TRUE));
        } // logoff
        else
        {
            NET_Packet P;
            P.w_begin(M_REMOTE_CONTROL_CMD);
            P.w_stringZ(arguments);

            Level().Send(P, net_flags(TRUE, TRUE));
        }
    }
    virtual void Save(IWriter* F){};
};

class CCC_SwapTeams : public IConsole_Command
{
public:
    CCC_SwapTeams(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server && Level().Server->GetGameState())
        {
            game_sv_TeamDeathmatch* tdmGame = smart_cast<game_sv_TeamDeathmatch*>(Level().Server->GetGameState());
            game_sv_CaptureTheArtefact* ctaGame =
                smart_cast<game_sv_CaptureTheArtefact*>(Level().Server->GetGameState());
            if (tdmGame)
            {
                BOOL old_team_swap = g_sv_tdm_bAutoTeamSwap;
                g_sv_tdm_bAutoTeamSwap = TRUE;
                tdmGame->AutoSwapTeams();
                g_sv_tdm_bAutoTeamSwap = old_team_swap;
            }
            else if (ctaGame)
            {
                ctaGame->SwapTeams();
            }
            else
            {
                Msg("! Current game type not support team swapping");
                return;
            }
            Level().Server->GetGameState()->SetRoundResult(eRoundEnd_GameRestartedFast);
            Level().Server->GetGameState()->OnRoundEnd();
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "swap teams for artefacthunt game"); }
};

class CCC_SvStatus : public IConsole_Command
{
public:
    CCC_SvStatus(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server && Level().Server->GetGameState())
        {
            Console->Execute("cfg_load all_server_settings");
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "Shows current server settings"); }
};

class CCC_SvChat : public IConsole_Command
{
public:
    CCC_SvChat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server && Level().Server->GetGameState())
        {
            game_sv_mp* game = smart_cast<game_sv_mp*>(Level().Server->GetGameState());
            if (game)
            {
                pstr msg;
                STRCONCAT(msg, args);
                if (xr_strlen(msg) > 256)
                {
                    msg[256] = 0;
                }
                game->SvSendChatMessage(msg);
            }
        }
    }
};

class CCC_MpStatistics : public IConsole_Command
{
public:
    CCC_MpStatistics(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
    virtual void Execute(LPCSTR args)
    {
        if (!OnServer())
            return;
        if (Level().Server && Level().Server->GetGameState())
        {
            Level().Server->GetGameState()->DumpOnlineStatistic();
        }
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "Shows current server settings"); }
};
class CCC_CompressorStatus : public IConsole_Command
{
public:
    CCC_CompressorStatus(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR args)
    {
        if (strstr(args, "info_full"))
            DumpNetCompressorStats(false);
        else if (strstr(args, "info"))
            DumpNetCompressorStats(true);
        else
            InvalidSyntax();
    }
    virtual void Info(TInfo& I) { xr_strcpy(I, "valid arguments is [info info_full on off]"); }
};

class CCC_TrafficOptimizationLevel : public CCC_Integer
{
    int dummy;

public:
    CCC_TrafficOptimizationLevel(pcstr name) : CCC_Integer(name, &dummy, 0, 7) {}

    void Execute(pcstr args) override
    {
        CCC_Integer::Execute(args);
        g_sv_traffic_optimization_level._and(g_sv_available_traffic_optimization_level, static_cast<u8>(dummy));
        dummy = g_sv_traffic_optimization_level.get();
    }
};

void register_mp_console_commands()
{
    CMD1(CCC_Restart, "g_restart");
    CMD1(CCC_RestartFast, "g_restart_fast");
    CMD1(CCC_Kill, "g_kill");

    // Net Interpolation
    CMD4(CCC_Float, "net_cl_interpolation", &g_cl_lvInterp, -1, 1);
    CMD4(CCC_Integer, "net_cl_icurvetype", &g_cl_InterpolationType, 0, 2);
    CMD4(CCC_Integer, "net_cl_icurvesize", (int*)&g_cl_InterpolationMaxPoints, 0, 2000);

    CMD1(CCC_Net_CL_Resync, "net_cl_resync");
    CMD1(CCC_Net_CL_ClearStats, "net_cl_clearstats");
    CMD1(CCC_Net_SV_ClearStats, "net_sv_clearstats");

// Network
#ifdef DEBUG
    CMD4(CCC_Integer, "net_cl_update_rate", &psNET_ClientUpdate, 20, 100);
    CMD4(CCC_Integer, "net_cl_pending_lim", &psNET_ClientPending, 0, 10);
#endif
    CMD4(CCC_Integer, "net_sv_update_rate", &psNET_ServerUpdate, 1, 100);
    CMD4(CCC_Integer, "net_sv_pending_lim", &psNET_ServerPending, 0, 10);
    CMD4(CCC_Integer, "net_sv_gpmode", &psNET_GuaranteedPacketMode, 0, 2);
    CMD3(CCC_Mask, "net_sv_log_data", &psNET_Flags, NETFLAG_LOG_SV_PACKETS);
    CMD3(CCC_Mask, "net_cl_log_data", &psNET_Flags, NETFLAG_LOG_CL_PACKETS);
#ifdef DEBUG
    CMD3(CCC_Mask, "net_dump_size", &psNET_Flags, NETFLAG_DBG_DUMPSIZE);
    CMD1(CCC_Dbg_NumObjects, "net_dbg_objects");
#endif // DEBUG
    CMD3(CCC_GSCDKey, "cdkey", gsCDKey, sizeof(gsCDKey));
    CMD4(CCC_Integer, "g_eventdelay", &g_dwEventDelay, 0, 1000);
    CMD4(CCC_Integer, "g_corpsenum", (int*)&g_dwMaxCorpses, 0, 100);

    CMD1(CCC_KickPlayerByName, "sv_kick"); // saved for backward compatibility
    CMD1(CCC_KickPlayerByID, "sv_kick_id");

    // CMD1(CCC_BanPlayerByName,	"sv_banplayer"				);
    CMD1(CCC_BanPlayerByCDKEY, "sv_banplayer");
    CMD1(CCC_BanPlayerByCDKEYDirectly, "sv_banplayer_by_digest");
    CMD1(CCC_BanPlayerByIP, "sv_banplayer_ip");
    CMD1(CCC_MakeScreenshot, "make_screenshot");
    CMD1(CCC_MakeConfigDump, "make_config_dump");
    CMD1(CCC_ScreenshotAllPlayers, "screenshot_all");
    CMD1(CCC_ConfigsDumpAll, "config_dump_all");

    CMD1(CCC_SetDemoPlaySpeed, "mpdemoplay_speed_set");
    CMD1(CCC_DemoPlayPauseOn, "mpdemoplay_pause_on");
    CMD1(CCC_DemoPlayCancelPauseOn, "mpdemoplay_cancel_pause_on");
    CMD1(CCC_DemoPlayRewindUntil, "mpdemoplay_rewind_until");
    CMD1(CCC_DemoPlayStopRewind, "mpdemoplay_stop_rewind");
    CMD1(CCC_DemoPlayRestart, "mpdemoplay_restart");
    CMD1(CCC_MulDemoPlaySpeed, "mpdemoplay_mulspeed");
    CMD1(CCC_DivDemoPlaySpeed, "mpdemoplay_divspeed");

#ifdef DEBUG
    CMD1(CCC_DbgMakeScreenshot, "dbg_make_screenshot");
#endif
    CMD4(CCC_Integer, "draw_downloads", &g_draw_downloads, 0, 1);
    CMD4(CCC_Integer, "sv_savescreenshots", &g_sv_mp_save_proxy_screenshots, 0, 1);
    CMD4(CCC_Integer, "sv_saveconfigs", &g_sv_mp_save_proxy_configs, 0, 1);

    CMD1(CCC_UnBanPlayerByIP, "sv_unbanplayer_ip");
    CMD1(CCC_UnBanPlayerByIndex, "sv_unbanplayer");

    CMD1(CCC_ListPlayers, "sv_listplayers");
    CMD1(CCC_ListPlayers_Banned, "sv_listplayers_banned");

    CMD1(CCC_ChangeGameType, "sv_changegametype");
    CMD1(CCC_ChangeLevel, "sv_changelevel");
    CMD1(CCC_ChangeLevelGameType, "sv_changelevelgametype");

    CMD1(CCC_AddMap, "sv_addmap");
    CMD1(CCC_ListMaps, "sv_listmaps");
    CMD1(CCC_NextMap, "sv_nextmap");
    CMD1(CCC_PrevMap, "sv_prevmap");
    CMD1(CCC_AnomalySet, "sv_nextanomalyset");

    CMD1(CCC_Vote_Start, "cl_votestart");
    CMD1(CCC_Vote_Stop, "sv_votestop");
    CMD1(CCC_Vote_Yes, "cl_voteyes");
    CMD1(CCC_Vote_No, "cl_voteno");

    CMD1(CCC_StartTimeEnvironment, "sv_setenvtime");

    CMD1(CCC_SetWeather, "sv_setweather");

    CMD4(CCC_Integer, "cl_cod_pickup_mode", &g_b_COD_PickUpMode, 0, 1);

    CMD4(CCC_Integer, "sv_remove_weapon", &g_iWeaponRemove, -1, 1);
    CMD4(CCC_Integer, "sv_remove_corpse", &g_iCorpseRemove, -1, 1);

    CMD4(CCC_Integer, "sv_statistic_collect", &g_bCollectStatisticData, 0, 1);
    CMD1(CCC_SaveStatistic, "sv_statistic_save");
//	CMD4(CCC_Integer,		"sv_statistic_save_auto", &g_bStatisticSaveAuto, 0, 1);

#ifndef MASTER_GOLD
    // Using CCC_AuthCheck twice, yes. It's not a mistake.
    CMD4(CCC_AuthCheck, "sv_no_auth_check", &g_SV_Disable_Auth_Check, 0, 1);
    CMD4(CCC_AuthCheck, "sv_ignore_version_mismatch", &g_sv_ignore_version_mismatch, 0, 1);
#endif // MASTER_GOLD

    CMD4(CCC_Integer, "sv_artefact_spawn_force", &g_SV_Force_Artefact_Spawn, 0, 1);

    CMD4(CCC_Integer, "net_dbg_dump_update_write", &g_Dump_Update_Write, 0, 1);
    CMD4(CCC_Integer, "net_dbg_dump_update_read", &g_Dump_Update_Read, 0, 1);

    CMD1(CCC_ReturnToBase, "sv_return_to_base");
    CMD1(CCC_GetServerAddress, "get_server_address");

#ifdef DEBUG
    CMD4(CCC_Integer, "sv_skip_winner_waiting", &g_sv_Skip_Winner_Waiting, 0, 1);

    CMD4(CCC_Integer, "sv_wait_for_players_ready", &g_sv_Wait_For_Players_Ready, 0, 1);
#endif
    CMD1(CCC_StartTeamMoney, "sv_startteammoney");

    CMD4(CCC_Integer, "sv_hail_to_winner_time", &G_DELAYED_ROUND_TIME, 0, 60);

    //. CMD4(CCC_Integer,		"sv_pending_wait_time",		&g_sv_Pending_Wait_Time, 0, 60000);

    CMD4(CCC_Integer, "sv_client_reconnect_time", (int*)&g_sv_Client_Reconnect_Time, 0, 60);

    CMD4(CCC_SV_Integer, "sv_rpoint_freeze_time", (int*)&g_sv_base_dwRPointFreezeTime, 0, 60000);
    CMD4(CCC_SV_Integer, "sv_vote_enabled", &g_sv_base_iVotingEnabled, 0, 0x00FF);

    CMD4(CCC_SV_Integer, "sv_spectr_freefly", (int*)&g_sv_mp_bSpectator_FreeFly, 0, 1);
    CMD4(CCC_SV_Integer, "sv_spectr_firsteye", (int*)&g_sv_mp_bSpectator_FirstEye, 0, 1);
    CMD4(CCC_SV_Integer, "sv_spectr_lookat", (int*)&g_sv_mp_bSpectator_LookAt, 0, 1);
    CMD4(CCC_SV_Integer, "sv_spectr_freelook", (int*)&g_sv_mp_bSpectator_FreeLook, 0, 1);
    CMD4(CCC_SV_Integer, "sv_spectr_teamcamera", (int*)&g_sv_mp_bSpectator_TeamCamera, 0, 1);
    CMD4(CCC_Integer, "cl_mpdemosave", (int*)&g_cl_save_demo, 0, 1);

    CMD4(CCC_SV_Integer, "sv_vote_participants", (int*)&g_sv_mp_bCountParticipants, 0, 1);
    CMD4(CCC_SV_Float, "sv_vote_quota", &g_sv_mp_fVoteQuota, 0.0f, 1.0f);
    CMD4(CCC_SV_Float, "sv_vote_time", &g_sv_mp_fVoteTime, 0.5f, 10.0f);

    CMD4(CCC_SV_Integer, "sv_forcerespawn", (int*)&g_sv_dm_dwForceRespawn, 0, 3600); // sec
    CMD4(CCC_SV_Integer, "sv_fraglimit", &g_sv_dm_dwFragLimit, 0, 1000);
    CMD4(CCC_SV_Integer, "sv_timelimit", &g_sv_dm_dwTimeLimit, 0, 180); // min
    CMD4(CCC_SV_Integer, "sv_dmgblockindicator", (int*)&g_sv_dm_bDamageBlockIndicators, 0, 1);
    CMD4(CCC_SV_Integer, "sv_dmgblocktime", (int*)&g_sv_dm_dwDamageBlockTime, 0, 600); // sec
    CMD4(CCC_SV_Integer, "sv_anomalies_enabled", (int*)&g_sv_dm_bAnomaliesEnabled, 0, 1);
    CMD4(CCC_SV_Integer, "sv_anomalies_length", (int*)&g_sv_dm_dwAnomalySetLengthTime, 0, 180); // min
    CMD4(CCC_SV_Integer, "sv_pda_hunt", (int*)&g_sv_dm_bPDAHunt, 0, 1);
    CMD4(CCC_SV_Integer, "sv_warm_up", (int*)&g_sv_dm_dwWarmUp_MaxTime, 0, 3600); // sec

    CMD4(CCC_Integer, "sv_max_ping_limit", (int*)&g_sv_dwMaxClientPing, 1, 2000);

    CMD4(CCC_SV_Integer, "sv_auto_team_balance", (int*)&g_sv_tdm_bAutoTeamBalance, 0, 1);
    CMD4(CCC_SV_Integer, "sv_auto_team_swap", (int*)&g_sv_tdm_bAutoTeamSwap, 0, 1);
    CMD4(CCC_SV_Integer, "sv_friendly_indicators", (int*)&g_sv_tdm_bFriendlyIndicators, 0, 1);
    CMD4(CCC_SV_Integer, "sv_friendly_names", (int*)&g_sv_tdm_bFriendlyNames, 0, 1);
    CMD4(CCC_SV_Float, "sv_friendlyfire", &g_sv_tdm_fFriendlyFireModifier, 0.0f, 2.0f);
    CMD4(CCC_SV_Integer, "sv_teamkill_limit", &g_sv_tdm_iTeamKillLimit, 0, 100);
    CMD4(CCC_SV_Integer, "sv_teamkill_punish", (int*)&g_sv_tdm_bTeamKillPunishment, 0, 1);

    CMD4(CCC_SV_Integer, "sv_artefact_respawn_delta", (int*)&g_sv_ah_dwArtefactRespawnDelta, 0, 600); // sec
    CMD4(CCC_SV_Integer, "sv_artefacts_count", (int*)&g_sv_ah_dwArtefactsNum, 1, 100);
    CMD4(CCC_SV_Integer, "sv_artefact_stay_time", (int*)&g_sv_ah_dwArtefactStayTime, 0, 180); // min
    CMD4(CCC_SV_Integer, "sv_reinforcement_time", (int*)&g_sv_ah_iReinforcementTime, -1, 3600); // sec
    CMD4(CCC_SV_Integer, "sv_bearercantsprint", (int*)&g_sv_ah_bBearerCantSprint, 0, 1);
    CMD4(CCC_SV_Integer, "sv_shieldedbases", (int*)&g_sv_ah_bShildedBases, 0, 1);
    CMD4(CCC_SV_Integer, "sv_returnplayers", (int*)&g_sv_ah_bAfReturnPlayersToBases, 0, 1);
    CMD1(CCC_SwapTeams, "g_swapteams");
#ifdef DEBUG
    CMD4(CCC_SV_Integer, "sv_ignore_money_on_buy", (int*)&g_sv_dm_bDMIgnore_Money_OnBuy, 0, 1);
#endif

    CMD1(CCC_RadminCmd, "ra");
    CMD1(CCC_Name, "name");
    CMD1(CCC_SvStatus, "sv_status");
    CMD1(CCC_SvChat, "chat");

    //-----------------
    CMD4(CCC_Integer, "sv_max_suspicious_actions", (int*)&g_sv_max_suspicious_actions, 1, 30);
    CMD3(CCC_Token, "sv_suspicious_actions_ban_time", &g_sv_suspicious_actions_ban_time, g_ban_times); // min

    CMD3(CCC_Token, "sv_adm_menu_ban_time", &g_sv_adm_menu_ban_time, g_ban_times); // min
    //	CMD4(CCC_Integer,		"sv_adm_menu_ban_time",			(int*)&g_sv_adm_menu_ban_time, 1, 60); //min
    CMD4(CCC_Integer, "sv_adm_menu_ping_limit", (int*)&g_sv_adm_menu_ping_limit, 1, 200); // min

    CMD4(CCC_Integer, "sv_invincible_time", (int*)&g_sv_cta_dwInvincibleTime, 0, 60); // sec
    CMD4(CCC_Integer, "sv_artefact_returning_time", (int*)&g_sv_cta_artefactReturningTime, 0, 5 * 60); // sec
    CMD4(CCC_Integer, "sv_activated_return", (int*)&g_sv_cta_activatedArtefactRet, 0, 1)
    CMD4(CCC_Integer, "sv_show_player_scores_time", (int*)&g_sv_cta_PlayerScoresDelayTime, 1, 20); // sec
    CMD4(CCC_Integer, "sv_cta_runkup_to_arts_div", (int*)&g_sv_cta_rankUpToArtsCountDiv, 0, 10);
    CMD1(CCC_CompressorStatus, "net_compressor_status");
    CMD4(CCC_SV_Integer, "net_compressor_enabled", (int*)&g_net_compressor_enabled, 0, 1);
    CMD4(CCC_SV_Integer, "net_compressor_gather_stats", (int*)&g_net_compressor_gather_stats, 0, 1);
    CMD1(CCC_MpStatistics, "sv_dump_online_statistics");
    CMD4(CCC_SV_Integer, "sv_dump_online_statistics_period", (int*)&g_sv_mp_iDumpStatsPeriod, 0, 60); // min
#ifdef DEBUG
    CMD4(CCC_SV_Integer, "cl_dbg_min_ping", (int*)&lag_simmulator_min_ping, 0, 1000);
    CMD4(CCC_SV_Integer, "cl_dbg_max_ping", (int*)&lag_simmulator_max_ping, 0, 1000);
#endif

    // GameSpy Presence and Messaging
    CMD1(CCC_CreateGameSpyAccount, "gs_create_account");
    CMD1(CCC_GapySpyListProfiles, "gs_list_profiles");
    CMD1(CCC_GameSpyLogin, "gs_login");
    CMD1(CCC_GameSpyLogout, "gs_logout");
    CMD1(CCC_GameSpyDeleteProfile, "gs_delete_profile");
    CMD1(CCC_GameSpyPrintProfile, "gs_print_profile");
    CMD1(CCC_GameSpySuggestUNicks, "gs_suggest_unicks");
    CMD1(CCC_GameSpyRegisterUniqueNick, "gs_register_unique_nick");
    CMD1(CCC_GameSpyProfile, "gs_profile");
    CMD4(CCC_Integer, "sv_write_update_bin", &g_sv_write_updates_bin, 0, 1);
    CMD1(CCC_TrafficOptimizationLevel, "sv_traffic_optimization_level");
}
