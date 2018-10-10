#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CBloodsuckerStateAttackHide : public CState<_Object>
{
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 m_target_node;

public:
    CBloodsuckerStateAttackHide(_Object* obj);

    virtual void reinit();

    virtual void initialize();
    virtual void reselect_state();
    virtual void finalize();
    virtual void critical_finalize();
    virtual bool check_completion();

    virtual void setup_substates();
    virtual void check_force_state();

private:
    void select_camp_point();
};

#include "bloodsucker_attack_state_hide_inline.h"
