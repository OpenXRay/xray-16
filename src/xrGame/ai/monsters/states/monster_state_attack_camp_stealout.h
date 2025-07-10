#pragma once

#include "ai/monsters/states/monster_state_move.h"

template <typename _Object>
class CStateMonsterAttackCampStealOut : public CMonsterStateMove<_Object>
{
    typedef CMonsterStateMove<_Object> inherited;

public:
    CStateMonsterAttackCampStealOut(_Object* obj);

    virtual void execute();
    virtual bool check_completion();
    virtual bool check_start_conditions();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_attack_camp_stealout_inline.h"
