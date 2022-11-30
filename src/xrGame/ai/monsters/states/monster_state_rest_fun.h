#pragma once

#include "ai/monsters/state.h"
#include "ai_debug.h"

template <typename _Object>
class CStateMonsterRestFun : public CState<_Object>
{
    static constexpr float IMPULSE_TO_CORPSE = 15.f;
    static constexpr u32 MIN_DELAY = 100;
    static constexpr u32 TIME_IN_STATE = 8000;

private:
    typedef CState<_Object> inherited;

    u32 time_last_hit;

public:
    CStateMonsterRestFun(_Object* obj);
    virtual void initialize();
    virtual void execute();
    virtual bool check_completion();
    virtual bool check_start_conditions();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_rest_fun_inline.h"
