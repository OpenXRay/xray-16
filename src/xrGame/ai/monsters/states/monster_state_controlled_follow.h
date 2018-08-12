#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterControlledFollow : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateMonsterControlledFollow(_Object* obj);
    virtual void reselect_state();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_controlled_follow_inline.h"
