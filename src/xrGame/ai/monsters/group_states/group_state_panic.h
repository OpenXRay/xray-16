#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateGroupPanic : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateGroupPanic(_Object* obj);
    virtual ~CStateGroupPanic();

    virtual void initialize();
    virtual void reselect_state();
    virtual void check_force_state();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "group_state_panic_inline.h"
