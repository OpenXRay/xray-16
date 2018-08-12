#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterAttackCamp : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 m_target_node;

public:
    CStateMonsterAttackCamp(_Object* obj);

    virtual void initialize();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
    virtual bool check_start_conditions();

    virtual void check_force_state();
    virtual void reselect_state();
    virtual void setup_substates();
};

#include "monster_state_attack_camp_inline.h"
