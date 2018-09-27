#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CStateAbstract CState<_Object>

TEMPLATE_SPECIALIZATION
CStateAbstract::CState(_Object* obj, void* data)
{
    reset();

    object = obj;
    _data = data;
}

TEMPLATE_SPECIALIZATION
CStateAbstract::~CState() { free_mem(); }
TEMPLATE_SPECIALIZATION
void CStateAbstract::reinit()
{
    if (current_substate != u32(-1))
        get_state_current()->critical_finalize();

    for (STATE_MAP_IT it = substates.begin(); it != substates.end(); it++)
        it->second->reinit();

    reset();
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::initialize()
{
    time_state_started = Device.dwTimeGlobal;

    current_substate = u32(-1); // means need reselect state
    prev_substate = u32(-1);
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::execute()
{
    VERIFY(object->g_Alive());
    // проверить внешние условия изменения состояния
    check_force_state();

    // если состояние не выбрано, перевыбрать
    if (current_substate == u32(-1))
    {
        reselect_state();

#ifdef DEBUG
        // Lain: added
        if (current_substate == u32(-1))
        {
            debug::text_tree tree;
            if (CBaseMonster* p_monster = smart_cast<CBaseMonster*>(object))
            {
                p_monster->add_debug_info(tree);
            }

            debug::log_text_tree(tree);
            VERIFY(current_substate != u32(-1));
        }
#endif
    }

    // выполнить текущее состояние
    CSState* state = get_state(current_substate);
    state->execute();

    // сохранить текущее состояние
    prev_substate = current_substate;

    // проверить на завершение текущего состояния
    if (state->check_completion())
    {
        state->finalize();
        current_substate = u32(-1);
    }
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::finalize() { reset(); }
TEMPLATE_SPECIALIZATION
void CStateAbstract::critical_finalize()
{
    if (current_substate != u32(-1))
        get_state_current()->critical_finalize();
    reset();
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::reset()
{
    current_substate = u32(-1);
    prev_substate = u32(-1);
    time_state_started = 0;
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::select_state(u32 new_state_id)
{
    if (current_substate == new_state_id)
        return;
    CSState* state;

    // если предыдущее состояние активно, завершить его
    if (current_substate != u32(-1))
    {
        state = get_state(current_substate);
        state->critical_finalize();
    }

    // установить новое состояние
    state = get_state(current_substate = new_state_id);

    // инициализировать новое состояние
    setup_substates();

    state->initialize();
}

TEMPLATE_SPECIALIZATION
CStateAbstract* CStateAbstract::get_state(u32 state_id)
{
    STATE_MAP_IT it = substates.find(state_id);
    VERIFY(it != substates.end());

    return it->second;
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::add_state(u32 state_id, CSState* s) { substates.insert(std::make_pair(state_id, s)); }
TEMPLATE_SPECIALIZATION
void CStateAbstract::free_mem()
{
    for (STATE_MAP_IT it = substates.begin(); it != substates.end(); it++)
        xr_delete(it->second);
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::fill_data_with(void* ptr_src, u32 size)
{
    VERIFY(ptr_src);
    VERIFY(_data);

    CopyMemory(_data, ptr_src, size);
}

#ifdef DEBUG

TEMPLATE_SPECIALIZATION
void CStateAbstract::add_debug_info(debug::text_tree& root_s)
{
    typedef debug::text_tree TextTree;
    if (!substates.size())
    {
        root_s.add_line("Current");
    }
    else
    {
        for (const auto& [id, substate] : substates)
        {
            TextTree& current_state_s = root_s.add_line(EMonsterState(id));
            if (current_substate == id)
            {
                if (substate)
                {
                    substate->add_debug_info(current_state_s);
                }
                else
                {
                    current_state_s.add_line("Current");
                }
            }
        }
    }
}

#endif

TEMPLATE_SPECIALIZATION
CStateAbstract* CStateAbstract::get_state_current()
{
    if (substates.empty() || (current_substate == u32(-1)))
        return 0;

    STATE_MAP_IT it = substates.find(current_substate);
    VERIFY(it != substates.end());

    return it->second;
}
TEMPLATE_SPECIALIZATION
EMonsterState CStateAbstract::get_state_type()
{
    if (substates.empty() || (current_substate == u32(-1)))
        return eStateUnknown;

    EMonsterState state = get_state_current()->get_state_type();
    return ((state == eStateUnknown) ? EMonsterState(current_substate) : state);
}

TEMPLATE_SPECIALIZATION
void CStateAbstract::remove_links(IGameObject* object)
{
    auto i = substates.begin();
    auto e = substates.end();
    for (; i != e; ++i)
        (*i).second->remove_links(object);
}

TEMPLATE_SPECIALIZATION
bool CStateAbstract::check_control_start_conditions(ControlCom::EControlType type)
{
    CState* child = get_state_current();
    if (child && !child->check_control_start_conditions(type))
    {
        return false;
    }

    return true;
}

#undef TEMPLATE_SPECIALIZATION
