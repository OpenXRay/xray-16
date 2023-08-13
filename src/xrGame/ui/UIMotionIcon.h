#pragma once
#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"

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
    EState m_current_state;
    xr_map<EState, CUIStatic*> m_states;
    CUIProgressBar* m_power_progress;

    CUIProgressShape* m_luminosity_progress_shape;
    CUIProgressShape* m_noise_progress_shape;
    CUIProgressBar* m_luminosity_progress_bar;
    CUIProgressBar* m_noise_progress_bar;

    struct _npc_visibility
    {
        u16 id;
        float value;
        bool operator==(const u16& _id) { return id == _id; }
        bool operator<(const _npc_visibility& m) const { return (value < m.value); }
    };
    xr_vector<_npc_visibility> m_npc_visibility;
    bool m_bchanged;
    float m_luminosity;
    float m_cur_pos;
    float m_relative_size{ 1.0f };

public:
    CUIMotionIcon();
    ~CUIMotionIcon() override;
    virtual void Update();
    virtual void Draw();
    bool Init();
    void AttachToMinimap(const Frect& rect);
    void ShowState(EState state);
    void SetPower(float Pos);
    void SetNoise(float Pos);
    void SetLuminosity(float newPos);
    void SetActorVisibility(u16 who_id, float value);
    void ResetVisibility();
    pcstr GetDebugType() override { return "CUIMotionIcon"; }
};
