#pragma once
#include "ai/monsters/state.h"

template <typename _Object>
class CStateMonsterAttackMoveToHomePoint : public CState<_Object>
{
protected:
    typedef CState<_Object> inherited;
    typedef CState<_Object>* state_ptr;

    u32 m_target_node;
    Fvector m_target_pos;
    bool m_skip_camp;
    TTime m_selected_target_time;

public:
    CStateMonsterAttackMoveToHomePoint(_Object* obj);

    virtual void initialize();
    virtual void finalize();
    virtual void critical_finalize();
    virtual void execute();
    virtual void remove_links(IGameObject* object) { inherited::remove_links(object); }
    virtual bool check_start_conditions();
    virtual bool check_completion();

private:
    void select_target();
    void clean();
};

#include "monster_state_home_point_attack_inline.h"
