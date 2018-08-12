#pragma once
#include "ai/monsters/control_combase.h"
#include "Include/xrRender/KinematicsAnimated.h"

class CPsyHitEffectorCam;
class CPsyHitEffectorPP;

class CControllerPsyHit : public CControl_ComCustom<>
{
    typedef CControl_ComCustom<> inherited;

    MotionID m_stage[4];
    u8 m_current_index;

    CPsyHitEffectorCam* m_effector_cam;
    CPsyHitEffectorPP* m_effector_pp;

    enum ESoundState
    {
        ePrepare,
        eStart,
        ePull,
        eHit,
        eNone
    } m_sound_state;

    float m_min_tube_dist;

    // internal flag if weapon was hidden
    bool m_blocked;

    u32 m_time_last_tube;

public:
    virtual void load(LPCSTR section);
    virtual void reinit();
    virtual void update_frame();
    virtual bool check_start_conditions();
    virtual void activate();
    virtual void deactivate();

    virtual void on_event(ControlCom::EEventType, ControlCom::IEventData*);

    void on_death();
    bool tube_ready() const;

private:
    void stop();

    void play_anim();
    void death_glide_start();
    void death_glide_end();

    void set_sound_state(ESoundState state);
    void hit();
    bool check_conditions_final();
    bool see_enemy();
};
