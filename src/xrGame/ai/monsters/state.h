#pragma once

#include "state_defs.h"
#include "control_com_defs.h"

// Lain: added
#ifdef DEBUG
#include "debug_text_tree.h"
#endif

inline TTime current_time() { return Device.dwTimeGlobal; }

template <typename _Object>
class CState
{
    typedef CState<_Object> CSState;

public:
    CState(_Object* obj, void* data = 0);
    virtual ~CState();

    virtual void reinit();
    virtual void remove_links(IGameObject* object) = 0;

    virtual void initialize();
    virtual void execute();
    virtual void finalize();
    virtual void critical_finalize();

    virtual void reset();

    virtual bool check_completion() { return false; }
    virtual bool check_start_conditions() { return true; }
    virtual void reselect_state() {}
    virtual void check_force_state() {}
    CSState* get_state(u32 state_id);
    CSState* get_state_current();

    void fill_data_with(void* ptr_src, u32 size);

    u32 time_started() { return time_state_started; }
    virtual bool check_control_start_conditions(ControlCom::EControlType type);

// Lain: added
#ifdef DEBUG
    virtual void add_debug_info(debug::text_tree& root_s);
#endif

protected:
    void select_state(u32 new_state_id);
    void add_state(u32 state_id, CSState* s);

    virtual void setup_substates() {}
    EMonsterState get_state_type();

    u32 current_substate;
    u32 prev_substate;

    u32 time_state_started;

    _Object* object;

    void* _data;

private:
    void free_mem();

    using SubStates = xr_map<u32, CSState*>;
    SubStates substates;
    using STATE_MAP_IT = typename xr_map<u32, CSState*>::iterator;
};

template <typename _Object>
class CStateMove : public CState<_Object>
{
protected:
    using inherited = CState<_Object>;

public:
    CStateMove(_Object* obj, void* data = 0) : inherited(obj, data) {}
    virtual ~CStateMove() {}
    virtual void initialize()
    {
        inherited::initialize();
        this->object->path().prepare_builder();
    }
};

#include "state_inline.h"
