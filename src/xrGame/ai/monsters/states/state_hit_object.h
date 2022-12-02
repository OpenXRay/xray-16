#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterHitObject : public CState<_Object>
{
    static constexpr u32 TIME_OUT_STATE = 1000;
    static constexpr u32 TIME_POINTBREAK = 500;
    static constexpr float TEST_ANGLE = PI_DIV_6;
    static constexpr float IMPULSE = 20.f;

private:
    typedef CState<_Object> inherited;

    xr_vector<IGameObject*> m_nearest_objects;
    CPhysicsShellHolder* target;
    bool m_hitted;

public:
    CStateMonsterHitObject(_Object* obj) : inherited(obj) {}
    virtual void initialize();
    virtual void execute();
    virtual bool check_start_conditions();
    virtual bool check_completion();
};

#include "state_hit_object_inline.h"
