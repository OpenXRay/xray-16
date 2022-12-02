#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterSquadRestFollow : public CState<_Object>
{
    static constexpr float STOP_DISTANCE = 2.f;
    static constexpr float STAY_DISTANCE = 5 * STOP_DISTANCE;

    static constexpr u32 MIN_TIME_OUT = 2000;
    static constexpr u32 MAX_TIME_OUT = 3000;

protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    Fvector last_point;

public:
    CStateMonsterSquadRestFollow(_Object* obj);
    virtual ~CStateMonsterSquadRestFollow();

    virtual void initialize();
    virtual void reselect_state();
    virtual void setup_substates();
    virtual void check_force_state();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_squad_rest_follow_inline.h"
