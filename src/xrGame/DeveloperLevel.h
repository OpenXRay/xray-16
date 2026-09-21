#pragma once

#include "Level.h"
#include "xrEngine/DeveloperScene.h"

class CDeveloperLevel : public CLevel
{
    using inherited = CLevel;

public:
    ~CDeveloperLevel() override;

    bool IsDeveloperLevel() const override;
    ALife::_TIME_ID GetEnvironmentGameTime() const override;

    bool Load(u32 dwNum) override;
    void OnFrame() override;
    bool Load_GameSpecific_Before() override;
    bool Load_GameSpecific_After() override;
    void IR_OnKeyboardPress(int key) override;

private:
    static constexpr float developer_day_time_seconds = 12.0f * 60.0f * 60.0f;
    static constexpr float interaction_range = 3.0f;
    static constexpr float interaction_impulse = 40.0f;
    static constexpr u32 required_props = 2;

    class DevPropSource
    {
    public:
        pcstr section;
        pcstr visual;
    };

    static bool DeveloperModelExists(pcstr visual, string_path& fileName);

    void CreateCollision();
    void SpawnProps();
    void UpdateProgress();
    void Interact();
    void RunDiagnostics();

    xray::render::DeveloperScene m_scene;
    u32 m_propIds[required_props]{ u32(-1), u32(-1) };
    u32 m_propSpawned{};
    xr_string m_propFailure;
    bool m_propsRequested{};
    bool m_actorLogged{};
    bool m_readyLogged{};
    u32 m_nextSummaryTime{};
    u32 m_dtCount{};
    float m_dtSum{};
    float m_dtMin{};
    float m_dtMax{};
};
