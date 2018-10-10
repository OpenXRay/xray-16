#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterAttackMelee : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterAttackMelee(_Object* obj);
    virtual ~CStateMonsterAttackMelee();

    virtual void execute();

    virtual bool check_completion();
    virtual bool check_start_conditions();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_attack_melee_inline.h"
