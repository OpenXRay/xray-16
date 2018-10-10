#pragma once

#include "ai/monsters/state.h"
#include "ai/monsters/states/monster_state_attack.h"

template <typename _Object>
class CStateBurerAttackMelee : public CStateMonsterAttack<_Object>
{
    typedef CStateMonsterAttack<_Object> inherited;

public:
    CStateBurerAttackMelee(_Object* obj);
    virtual bool check_start_conditions();
    virtual bool check_completion();
};

#include "burer_state_attack_melee_inline.h"
