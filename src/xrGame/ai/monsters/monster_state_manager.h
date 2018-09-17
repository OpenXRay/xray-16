#pragma once
#include "state_manager.h"
#include "state.h"

// Lain: added
#ifdef DEBUG
#include "debug_text_tree.h"
#endif

template <typename _Object>
class CMonsterStateManager : public IStateManagerBase, public CState<_Object>
{
    typedef CState<_Object> inherited;

public:
    CMonsterStateManager(_Object* obj) : inherited(obj) {}
    virtual void reinit();
    virtual void update();
    virtual void force_script_state(EMonsterState state);
    virtual void execute_script_state();
    virtual void critical_finalize();
    virtual void remove_links(IGameObject* object) = 0;
    virtual EMonsterState get_state_type();

    virtual bool check_control_start_conditions(ControlCom::EControlType type)
    {
        return inherited::check_control_start_conditions(type);
    }

// Lain: added
#ifdef DEBUG
    virtual void add_debug_info(debug::text_tree& root_s);
#endif

protected:
    bool can_eat();
    bool check_state(u32 state_id);
};

#include "monster_state_manager_inline.h"
