#pragma once
#include "ai/monsters/basemonster/base_monster.h"

class CSnork : public CBaseMonster
{
    typedef CBaseMonster inherited;

    SVelocityParam m_fsVelocityJumpPrepare;
    SVelocityParam m_fsVelocityJumpGround;

public:
    CSnork();
    virtual ~CSnork();

    virtual void Load(LPCSTR section);
    virtual void reinit();
    virtual void UpdateCL();
    virtual void CheckSpecParams(u32 spec_params);
    virtual void jump(const Fvector& position, float factor);
    virtual bool ability_jump_over_physics() { return true; }
    virtual bool ability_distant_feel() { return true; }
    virtual void HitEntityInJump(const CEntity* pEntity);

    bool find_geometry(Fvector& dir);
    float trace(const Fvector& dir);

    bool trace_geometry(const Fvector& d, float& range);

    virtual bool check_start_conditions(ControlCom::EControlType type);
    virtual void on_activate_control(ControlCom::EControlType);
    pcstr get_monster_class_name() override { return "snork"; }
    virtual bool run_home_point_when_enemy_inaccessible() const { return false; }

private:
#ifdef _DEBUG
    virtual void debug_on_key(int key);
#endif

public:
    u32 m_target_node;
    bool start_threaten;
};
