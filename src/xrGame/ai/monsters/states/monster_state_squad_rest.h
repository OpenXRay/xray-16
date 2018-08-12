#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterSquadRest : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 time_next_state_reselect;

public:
    CStateMonsterSquadRest(_Object* obj);
    virtual ~CStateMonsterSquadRest();

    virtual void reselect_state();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_squad_rest_inline.h"
