#pragma once

template <typename _Object>
class CStateControlCamp : public CState<_Object>
{
    typedef CState<_Object> inherited;
    using inherited::object;
    using inherited::time_state_started;

    float m_angle_from;
    float m_angle_to;

    float m_target_angle;
    u32 m_time_next_updated;

public:
    CStateControlCamp(_Object* obj) : inherited(obj) {}
    virtual ~CStateControlCamp() {}
    virtual void initialize();
    virtual void execute();
    virtual bool check_completion();
    virtual bool check_start_conditions();

    virtual void remove_links(IGameObject* object) {}
private:
    virtual void update_target_angle();
};

#include "controller_state_attack_camp_inline.h"
