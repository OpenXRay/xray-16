#pragma once

#include "ai/monsters/basemonster/base_monster.h"
#include "ai/monsters/controlled_entity.h"

class CAI_Boar : public CBaseMonster, public CControlledEntity<CAI_Boar>
{
    typedef CBaseMonster inherited;
    typedef CControlledEntity<CAI_Boar> CControlled;

public:
    CAI_Boar();
    virtual ~CAI_Boar();

    virtual void Load(LPCSTR section);
    virtual BOOL net_Spawn(CSE_Abstract* DC);
    virtual void reinit();

    virtual void UpdateCL();

    virtual bool CanExecRotationJump() { return true; }
    virtual void CheckSpecParams(u32 spec_params);

    // look at enemy
    static void BoneCallback(CBoneInstance* B);

    float _velocity;
    float _cur_delta, _target_delta;
    bool look_at_enemy;

    virtual bool ability_can_drag() { return true; }
    pcstr get_monster_class_name() override { return "boar"; }
};
