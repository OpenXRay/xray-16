#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterControlledFollow : public CState<_Object>
{
    static constexpr float STOP_DISTANCE = 2.f;
    static constexpr float STAY_DISTANCE = 5.f * STOP_DISTANCE;

    static constexpr u32 MIN_TIME_OUT = 4000;
    static constexpr u32 MAX_TIME_OUT = 6000;

private:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateMonsterControlledFollow(_Object* obj);
    virtual void reselect_state();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_controlled_follow_inline.h"
