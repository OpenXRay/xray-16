#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStatePoltergeistAttackHidden : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStatePoltergeistAttackHidden(_Object* obj);
    virtual ~CStatePoltergeistAttackHidden() {}
    virtual void initialize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    bool check_home_point();

private:
    void select_target_for_move();

    u32 m_fly_side_select_tick;
    float m_fly_radius_factor;
    bool m_fly_left;
    Fvector m_target;
    u32 m_target_vertex;
};

#include "poltergeist_state_attack_hidden_inline.h"
