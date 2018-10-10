#pragma once

#include "ai/monsters/state.h"
#include "ai_debug.h"

template <typename _Object>
class CStateControllerAttack : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateControllerAttack(_Object* obj);
    virtual ~CStateControllerAttack() {}
    virtual void initialize();
    virtual void finalize();
    virtual void critical_finalize();

    virtual void execute();
    virtual void setup_substates();
    virtual void check_force_state();
    virtual void remove_links(IGameObject*) {}
    bool check_home_point();
};

#include "controller_state_attack_inline.h"
