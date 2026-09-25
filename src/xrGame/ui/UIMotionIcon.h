#pragma once

#include "xrUICore/Static/UIStatic.h"

class CUIProgressBar;
class CUIProgressShape;

class CUIMotionIcon final : public CUIStatic
{
    using inherited = CUIStatic;

public:
    enum EState
    {
        stNormal,
        stCrouch,
        stCreep,
        stClimb,
        stRun,
        stSprint,
        stLast
    };

private:
    EState m_current_state{ stLast };
    xr_map<EState, CUIStatic*> m_states;
    CUIProgressBar* m_power_progress{};

    CUIProgressShape* m_luminosity_progress_shape{};
    CUIProgressShape* m_noise_progress_shape{};
    CUIProgressBar* m_luminosity_progress_bar{};
    CUIProgressBar* m_noise_progress_bar{};

    float m_luminosity{};
    float m_cur_pos{};
    float m_relative_size{ 1.0f };

public:
    CUIMotionIcon();
    void Update() override;
    void Draw() override;
    bool Init();
    void AttachToMinimap(const Frect& rect);
    void ShowState(EState state);
    void SetPower(float Pos);
    void SetNoise(float Pos);
    void SetLuminosity(float newPos, bool absolute = true);
    pcstr GetDebugType() override { return "CUIMotionIcon"; }
};
