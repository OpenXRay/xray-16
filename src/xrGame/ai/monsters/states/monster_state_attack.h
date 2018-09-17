#pragma once

#include "ai/monsters/state.h"
#include "ai_debug.h"

template <typename _Object>
class CStateMonsterAttack : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;
    using inherited::object;

    u32 m_time_next_run_away;
    u32 m_time_start_check_behinder;
    u32 m_time_start_behinder;

public:
    CStateMonsterAttack(_Object* obj);
    CStateMonsterAttack(_Object* obj, state_ptr state_move2home);
    CStateMonsterAttack(_Object* obj, state_ptr state_run, state_ptr state_melee);
    virtual ~CStateMonsterAttack();

    virtual void initialize();
    virtual void execute();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
protected:
    bool check_steal_state();
    bool check_find_enemy_state();
    bool check_run_away_state();
    bool check_run_attack_state();
    bool check_camp_state();
    bool check_home_point();
    bool check_behinder();
};

#include "monster_state_attack_inline.h"
