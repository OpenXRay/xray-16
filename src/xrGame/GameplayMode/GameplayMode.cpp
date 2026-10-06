#include "StdAfx.h"
#include "GameplayMode.h"

namespace XRay
{

CGameplayModeManager* CGameplayModeManager::m_instance = nullptr;

CGameplayModeManager& CGameplayModeManager::Instance()
{
    if (!m_instance)
        m_instance = xr_new<CGameplayModeManager>();
    return *m_instance;
}

CGameplayModeManager::CGameplayModeManager()
{
    m_config_path = "gameplay_mode.ini";
    m_settings.mode = EGameplayMode::Diplomatic;
    m_settings.show_violent_content_warning = true;
    m_settings.diplomacy_tutorials_completed = false;

    LoadSettings();
}

CGameplayModeManager::~CGameplayModeManager()
{
    SaveSettings();
    delete_data(m_instance);
}

EGameplayMode CGameplayModeManager::CurrentMode() const
{
    return m_settings.mode;
}

void CGameplayModeManager::SetGameplayMode(EGameplayMode mode)
{
    m_settings.mode = mode;
    SaveSettings();
}

bool CGameplayModeManager::IsDiplomaticMode() const
{
    return m_settings.mode == EGameplayMode::Diplomatic;
}

bool CGameplayModeManager::IsOriginalMode() const
{
    return m_settings.mode == EGameplayMode::Original;
}

bool CGameplayModeManager::ShouldLoadViolentContent() const
{
    return m_settings.mode == EGameplayMode::Original;
}

bool CGameplayModeManager::ShouldUseHologramProjectors() const
{
    return m_settings.mode == EGameplayMode::Diplomatic;
}

bool CGameplayModeManager::ShouldLoadWeapons() const
{
    return !IsDiplomaticMode();
}

bool CGameplayModeManager::LoadSettings(cpcstr config_path)
{
    if (config_path)
        m_config_path = config_path;

    // Default-safe behavior: diplomatic mode is selected unless overridden.
    m_settings.mode = EGameplayMode::Diplomatic;
    m_settings.show_violent_content_warning = true;
    m_settings.diplomacy_tutorials_completed = false;

    // TODO: Load ini values from engine settings.
    // This keeps the design conservative and compatible with vanilla startup.
    return true;
}

bool CGameplayModeManager::SaveSettings() const
{
    // TODO: Save to user config or registry.
    return true;
}

const SGameplaySettings& CGameplayModeManager::GetSettings() const
{
    return m_settings;
}

} // namespace XRay
