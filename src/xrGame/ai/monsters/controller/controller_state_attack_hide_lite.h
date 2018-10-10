#pragma once

// Hiding until enemy get out from its sight
template <typename _Object>
class CStateControlHideLite : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;
    using inherited::object;

    struct
    {
        Fvector position;
        u32 node;
    } target;

    u32 m_time_finished;

public:
    CStateControlHideLite(_Object* obj) : inherited(obj) {}
    virtual ~CStateControlHideLite() {}
    virtual void reinit();

    virtual void initialize();
    virtual void execute();

    virtual void finalize();

    virtual bool check_completion();
    virtual bool check_start_conditions();
    virtual void remove_links(IGameObject* object) {}
private:
    void select_target_point();
};

#include "controller_state_attack_hide_lite_inline.h"
