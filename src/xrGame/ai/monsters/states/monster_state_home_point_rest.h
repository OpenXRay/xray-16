#pragma once

#include "ai/monsters/states/monster_state_move.h"

template <typename _Object>
class CStateMonsterRestMoveToHomePoint : public CMonsterStateMove<_Object>
{
protected:
    typedef CMonsterStateMove<_Object> inherited;
    typedef CMonsterStateMove<_Object>* state_ptr;

    u32 m_target_node;

public:
    CStateMonsterRestMoveToHomePoint(_Object* obj) : inherited(obj), m_target_node(0) {}
    virtual void initialize();
    virtual void execute();
    virtual bool check_start_conditions();
    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_home_point_rest_inline.h"
