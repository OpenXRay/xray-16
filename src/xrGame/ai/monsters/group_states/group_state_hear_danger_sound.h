#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateGroupHearDangerousSound : public CState<_Object>
{
    static constexpr float LEADER_RADIUS = 20.f;
    static constexpr u32 FIND_POINT_ATTEMPTS = 5;

protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 m_target_node;

public:
    CStateGroupHearDangerousSound(_Object* obj);
    virtual ~CStateGroupHearDangerousSound() {}
    virtual void initialize();
    virtual void reselect_state();
    virtual void setup_substates();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
};

#include "group_state_hear_danger_sound_inline.h"
