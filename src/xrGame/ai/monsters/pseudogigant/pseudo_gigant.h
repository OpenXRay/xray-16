#pragma once
#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"

class CPseudoGigant : public CBaseMonster, public CControlledEntity<CPseudoGigant>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CPseudoGigant> CControlled;

private:
    xr_vector<IGameObject*> m_nearest;

    // step_effector
    struct
    {
        float time;
        float amplitude;
        float period_number;
    } step_effector;

    SAttackEffector m_threaten_effector;
    ref_sound m_sound_threaten_hit; // звук, который играется в голове у актера
    ref_sound m_sound_start_threaten; // звук, который играется в голове у актера

    u32 m_time_next_threaten;

    u32 m_threaten_delay_min;
    u32 m_threaten_delay_max;
    float m_threaten_dist_min;
    float m_threaten_dist_max;

    float m_kick_damage;

    u32 m_time_kick_actor_slow_down;

    SVelocityParam m_fsVelocityJumpPrepare;
    SVelocityParam m_fsVelocityJumpGround;

    LPCSTR m_kick_particles;

public:
    CPseudoGigant();
    virtual ~CPseudoGigant();

    virtual void Load(LPCSTR section);
    virtual void reinit();

    virtual bool ability_earthquake() { return true; }
    virtual void event_on_step();

    virtual bool check_start_conditions(ControlCom::EControlType type);
    virtual void on_activate_control(ControlCom::EControlType);

    virtual void on_threaten_execute();

    virtual void HitEntityInJump(const CEntity* pEntity);
    virtual void TranslateActionToPathParams();
    pcstr get_monster_class_name() override { return "pseudogigant"; }
};
