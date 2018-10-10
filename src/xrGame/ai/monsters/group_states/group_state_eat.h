#pragma once

#include "ai/monsters/state.h"

template <typename _Object>
class CStateGroupEat : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    const CEntityAlive* corpse;

    u32 m_time_last_eat;

public:
    CStateGroupEat(_Object* obj);
    virtual ~CStateGroupEat();

    virtual void reinit();
    virtual void initialize();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object);

    virtual void reselect_state();
    virtual void setup_substates();
    virtual bool check_completion();
    virtual bool check_start_conditions();

private:
    bool hungry();
};

#include "group_state_eat_inline.h"
