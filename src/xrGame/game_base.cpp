#include "pch_script.h"
#include "game_base.h"
#include "ai_space.h"
#include "Level.h"
#include "xrMessages.h"

u64 g_qwStartGameTime = 12 * 60 * 60 * 1000;
float g_fTimeFactor = pSettings->r_float("alife", "time_factor");
u64 g_qwEStartGameTime = 12 * 60 * 60 * 1000;

game_GameState::game_GameState()
{
	m_type = eGameIDSingle;
	m_phase = GAME_PHASE_NONE;

    VERIFY(g_pGameLevel);
    m_qwStartProcessorTime = Level().timeServer_Async();
    m_qwStartGameTime = g_qwStartGameTime;
    m_fTimeFactor = g_fTimeFactor;
    m_qwEStartProcessorTime = m_qwStartProcessorTime;
    m_qwEStartGameTime = g_qwEStartGameTime;
    m_fETimeFactor = m_fTimeFactor;
}

CLASS_ID game_GameState::getCLASS_ID(LPCSTR game_type_name, bool isServer)
{
    EGameIDs gameID = ParseStringToGameType(game_type_name);
    switch (gameID)
    {
    case eGameIDSingle: return (isServer) ? TEXT2CLSID("SV_SINGL") : TEXT2CLSID("CL_SINGL"); break;

    case eGameIDDeathmatch: return (isServer) ? TEXT2CLSID("SV_DM") : TEXT2CLSID("CL_DM"); break;

    case eGameIDTeamDeathmatch: return (isServer) ? TEXT2CLSID("SV_TDM") : TEXT2CLSID("CL_TDM"); break;

    case eGameIDArtefactHunt: return (isServer) ? TEXT2CLSID("SV_AHUNT") : TEXT2CLSID("CL_AHUNT"); break;

    case eGameIDCaptureTheArtefact: return (isServer) ? TEXT2CLSID("SV_CTA") : TEXT2CLSID("CL_CTA"); break;

    default: return (TEXT2CLSID("")); break;
    }
}

void game_GameState::switch_Phase(u32 new_phase)
{
    OnSwitchPhase(m_phase, new_phase);

    m_phase = u16(new_phase);
    m_start_time = Level().timeServer();
}

ALife::_TIME_ID game_GameState::GetStartGameTime() { return (m_qwStartGameTime); }
ALife::_TIME_ID game_GameState::GetGameTime()
{
    return (m_qwStartGameTime +
        ALife::_TIME_ID(m_fTimeFactor * float(Level().timeServer_Async() - m_qwStartProcessorTime)));
}

float game_GameState::GetGameTimeFactor() { return (m_fTimeFactor); }
void game_GameState::SetGameTimeFactor(const float fTimeFactor)
{
    m_qwStartGameTime = GetGameTime();
    m_qwStartProcessorTime = Level().timeServer_Async();
    m_fTimeFactor = fTimeFactor;
}

void game_GameState::SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
    m_qwStartGameTime = GameTime;
    m_qwStartProcessorTime = Level().timeServer_Async();
    m_fTimeFactor = fTimeFactor;
}

ALife::_TIME_ID game_GameState::GetEnvironmentGameTime()
{
    return (m_qwEStartGameTime +
        ALife::_TIME_ID(m_fETimeFactor * float(Level().timeServer_Async() - m_qwEStartProcessorTime)));
}

float game_GameState::GetEnvironmentGameTimeFactor() { return (m_fETimeFactor); }
void game_GameState::SetEnvironmentGameTimeFactor(const float fTimeFactor)
{
    m_qwEStartGameTime = GetEnvironmentGameTime();
    m_qwEStartProcessorTime = Level().timeServer_Async();
    m_fETimeFactor = fTimeFactor;
}

void game_GameState::SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
    m_qwEStartGameTime = GameTime;
    m_qwEStartProcessorTime = Level().timeServer_Async();
    m_fETimeFactor = fTimeFactor;
}
