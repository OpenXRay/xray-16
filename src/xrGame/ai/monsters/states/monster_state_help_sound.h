#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterHearHelpSound : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

public:
    CStateMonsterHearHelpSound(_Object* obj);
    virtual ~CStateMonsterHearHelpSound() {}
    virtual void reselect_state();
    virtual void setup_substates();

    virtual bool check_start_conditions();
    virtual bool check_completion();

    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "monster_state_help_sound_inline.h"
