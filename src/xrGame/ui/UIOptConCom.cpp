#include "StdAfx.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"
#include "UIOptConCom.h"
#include "xrCore/xrCore.h"
#include "gametype_chooser.h"
#include "RegistryFuncs.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "xrGameSpy/GameSpy_GP.h"
#include "ui/UICDkey.h"

xr_token g_GameModes[] = {{"st_deathmatch", eGameIDDeathmatch}, {"st_team_deathmatch", eGameIDTeamDeathmatch},
    {"st_artefacthunt", eGameIDArtefactHunt}, {"st_capture_the_artefact", eGameIDCaptureTheArtefact}, {0, 0}};

CUIOptConCom::CUIOptConCom() { xr_strcpy(m_playerName, ""); }
class CCC_UserName : public CCC_String
{
public:
    CCC_UserName(LPCSTR N, LPSTR V, int _size) : CCC_String(N, V, _size) { bEmptyArgsHandled = false; };
    virtual void Execute(LPCSTR arguments)
    {
        string512 str;
        xr_strcpy(str, arguments);

        u32 const max_name_length = GP_UNIQUENICK_LEN - 1;
        if (xr_strlen(str) > max_name_length)
            str[max_name_length] = 0;

        CCC_String::Execute(str);

        WritePlayerName_ToRegistry(value);
    }
    virtual void Save(IWriter* F){};
};

void CUIOptConCom::Init()
{
    ReadPlayerNameFromRegistry();
    CMD3(CCC_UserName, "mm_net_player_name", m_playerName, 64);

    m_iMaxPlayers = 32;
    m_curGameMode = eGameIDDeathmatch;
    CMD4(CCC_Integer, "mm_net_srv_maxplayers", &m_iMaxPlayers, 2, 32);
    CMD3(CCC_Token, "mm_net_srv_gamemode", &m_curGameMode, g_GameModes);
    m_uNetSrvParams.zero();
    CMD3(CCC_Mask, "mm_mm_net_srv_dedicated", &m_uNetSrvParams, flNetSrvDedicated);
    CMD3(CCC_Mask, "mm_net_con_publicserver", &m_uNetSrvParams, flNetConPublicServer);
    CMD3(CCC_Mask, "mm_net_con_spectator_on", &m_uNetSrvParams, flNetConSpectatorOn);
    m_iNetConSpectator = 20;
    CMD4(CCC_Integer, "mm_net_con_spectator", &m_iNetConSpectator, 1, 32);

    xr_strcpy(reinforcementType, "reinforcement");
    CMD3(CCC_String, "mm_net_srv_reinforcement_type", reinforcementType, sizeof(reinforcementType));

    m_fNetWeatherRate = 1.0f;
    CMD4(CCC_Float, "mm_net_weather_rateofchange", &m_fNetWeatherRate, 0.0, 100.0f);

    xr_strcpy(m_serverName, "Stalker");
    CMD3(CCC_String, "mm_net_srv_name", m_serverName, sizeof(m_serverName));

    m_uNetFilter.one();
    CMD3(CCC_Mask, "mm_net_filter_empty", &m_uNetFilter, fl_empty);
    CMD3(CCC_Mask, "mm_net_filter_full", &m_uNetFilter, fl_full);
    CMD3(CCC_Mask, "mm_net_filter_pass", &m_uNetFilter, fl_pass);
    CMD3(CCC_Mask, "mm_net_filter_wo_pass", &m_uNetFilter, fl_wo_pass);
    CMD3(CCC_Mask, "mm_net_filter_wo_ff", &m_uNetFilter, fl_wo_ff);
    CMD3(CCC_Mask, "mm_net_filter_listen", &m_uNetFilter, fl_listen);
};

void CUIOptConCom::ReadPlayerNameFromRegistry() { GetPlayerName_FromRegistry(m_playerName, sizeof(m_playerName)); };
void CUIOptConCom::WritePlayerNameToRegistry() { WritePlayerName_ToRegistry(m_playerName); };
