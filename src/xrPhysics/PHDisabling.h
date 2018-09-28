#pragma once

#include "DisablingParams.h"
#include <ode/common.h>

struct SDisableVector
{
    Fvector sum;
    Fvector previous;
    float UpdatePrevious(const Fvector& new_vector);
    float Update(const Fvector& new_vector);
    void Reset();
    void Init();
    float SumMagnitude();

    SDisableVector();
};
struct SDisableUpdateState
{
    bool disable;
    bool enable;
    void Reset();
    SDisableUpdateState& operator&=(SDisableUpdateState& ltate);
    SDisableUpdateState();
};

struct CBaseDisableData
{
    CBaseDisableData();

protected:
    u16 m_count;
    u16 m_frames;
    u16 m_last_frame_updated;
    SDisableUpdateState m_stateL1;
    SDisableUpdateState m_stateL2;
    bool m_disabled;

protected:
    IC void CheckState(const SDisableUpdateState& state)
    {
        if (m_disabled)
            m_disabled = !state.enable;
        else
            m_disabled = state.disable;
    }

    void Disabling();
    void Reinit();
    virtual void Disable() = 0;
    virtual void ReEnable() = 0;
    virtual void UpdateL1() = 0;
    virtual void UpdateL2() = 0;
    virtual dBodyID get_body() = 0;
};

class CPHDisablingBase : public virtual CBaseDisableData
{
public:
    void UpdateValues(const Fvector& new_pos, const Fvector& new_vel);
    virtual void UpdateL2();
    virtual void set_DisableParams(const SOneDDOParams& params);

protected:
    void Reinit();
    IC void CheckState(SDisableUpdateState& state, float vel, float accel)
    {
        if (vel < m_params.velocity && accel < m_params.acceleration)
            state.disable = true;
        if (vel > m_params.velocity * worldDisablingParams.reanable_factor ||
            accel > m_params.acceleration * worldDisablingParams.reanable_factor)
            state.enable = true;
    }

protected:
    SDisableVector m_mean_velocity;
    SDisableVector m_mean_acceleration;
    SOneDDOParams m_params;
};

class CPHDisablingRotational : public CPHDisablingBase
{
public:
    CPHDisablingRotational();
    void Reinit();
    virtual void UpdateL1();
    virtual void set_DisableParams(const SAllDDOParams& params);
};

class CPHDisablingTranslational : public CPHDisablingBase
{
public:
    CPHDisablingTranslational();
    void Reinit();
    virtual void UpdateL1();
    virtual void set_DisableParams(const SAllDDOParams& params);
};

class CPHDisablingFull : public CPHDisablingTranslational, public CPHDisablingRotational
{
public:
    void Reinit();
    virtual void UpdateL1();
    virtual void UpdateL2();
    virtual void set_DisableParams(const SAllDDOParams& params);
};
