#pragma once
#include "ai/monsters/state.h"

template <typename Object>
class CStateBurerAntiAim : public CState<Object>
{
private:
    typedef CState<Object> inherited;

public:
    CStateBurerAntiAim(Object* obj);

    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_start_conditions();
    virtual bool check_completion();

private:
    TTime m_last_shield_started;
    TTime m_next_particle_allowed;
    float m_shield_start_anim_length_sec;
    bool m_started;
    bool m_allow_anti_aim;
};

#include "burer_state_attack_antiaim_inline.h"
