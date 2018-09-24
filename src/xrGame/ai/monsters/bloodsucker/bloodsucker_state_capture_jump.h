#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateCaptureJumpBloodsucker : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;
    using inherited::prev_substate;
    using inherited::current_substate;
    using inherited::get_state_current;
    using inherited::check_home_point;
    using inherited::check_find_enemy;
    using inherited::select_state;

public:
    CStateCaptureJumpBloodsucker(_Object* obj);
    virtual ~CStateCaptureJumpBloodsucker();

    virtual void execute();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "bloodsucker_state_capture_jump_inline.h"
