#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateGroupAttackMoveToHomePoint : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;
    using inherited::object;
    using inherited::prev_substate;
    using inherited::current_substate;
    using inherited::select_state;
    using inherited::get_state_current;

    u32 m_target_node;
    bool m_skip_camp;

    TTime m_first_tick_enemy_inaccessible;
    TTime m_last_tick_enemy_inaccessible;
    TTime m_state_started;

public:
    CStateGroupAttackMoveToHomePoint(_Object* obj);
    virtual void initialize();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_start_conditions();
    virtual bool check_completion();

    virtual void reselect_state();
    virtual void setup_substates();

    bool enemy_inaccessible();
};

#include "group_state_home_point_attack_inline.h"
