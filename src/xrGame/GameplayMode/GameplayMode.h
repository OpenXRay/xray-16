#pragma once

#include "xrCore/xrCore.h"
#include "xrCore/string.h"

class CGameObject;

namespace XRay
{

enum class EGameplayMode
{
    Diplomatic = 0,
    Original = 1,
};

struct SGameplaySettings
{
    EGameplayMode mode = EGameplayMode::Diplomatic;
    bool show_violent_content_warning = true;
    bool diplomacy_tutorials_completed = false;
};

class CGameplayModeManager
{
public:
    static CGameplayModeManager& Instance();

    EGameplayMode CurrentMode() const;
    void SetGameplayMode(EGameplayMode mode);

    bool IsDiplomaticMode() const;
    bool IsOriginalMode() const;
    bool ShouldLoadViolentContent() const;
    bool ShouldUseHologramProjectors() const;
    bool ShouldLoadWeapons() const;

    bool LoadSettings(cpcstr config_path = nullptr);
    bool SaveSettings() const;
    const SGameplaySettings& GetSettings() const;

private:
    CGameplayModeManager();
    ~CGameplayModeManager();
    CGameplayModeManager(const CGameplayModeManager&) = delete;
    CGameplayModeManager& operator=(const CGameplayModeManager&) = delete;

    static CGameplayModeManager* m_instance;

    SGameplaySettings m_settings;
    shared_str m_config_path;
};

} // namespace XRay
