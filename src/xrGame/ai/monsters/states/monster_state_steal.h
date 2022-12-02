#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterSteal : public CState<_Object>
{
    static constexpr float STEAL_MIN_DISTANCE = 4.f;
    static constexpr float STEAL_MAX_DISTANCE = 15.f;
    static constexpr float STEAL_MAX_PATH_ANGLE = PI_DIV_6;

private:
    typedef CState<_Object> inherited;

public:
    CStateMonsterSteal(_Object* obj);

    virtual void initialize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_completion();
    virtual bool check_start_conditions();

private:
    bool check_conditions();
};

#include "monster_state_steal_inline.h"
