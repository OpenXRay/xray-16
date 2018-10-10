#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterControlled : public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CStateMonsterControlled(_Object* obj);
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_controlled_inline.h"
