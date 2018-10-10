#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateBloodsuckerVampireHide : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;
    using inherited::object;
    using inherited::prev_substate;
    using inherited::current_substate;
    using inherited::select_state;
    using inherited::get_state;
    using inherited::get_state_current;

public:
    CStateBloodsuckerVampireHide(_Object* obj);

    virtual void reselect_state();
    virtual void setup_substates();
    virtual bool check_completion();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "bloodsucker_vampire_hide_inline.h"
