#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _Object\
>

#define CMonsterStateManagerAbstract CMonsterStateManager<_Object>

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::reinit() { inherited::reinit(); }
namespace detail
{ // helper function implemented in file alife_simulator.cpp
bool object_exists_in_alife_registry(u32 id);
} // namespace detail

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::remove_links(IGameObject* object)
{
    inherited::remove_links(object);
}

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::update()
{
    // Lain: added
    if (!detail::object_exists_in_alife_registry(this->object->ID()))
    {
        return;
    }

    if (!this->object->g_Alive())
    {
        return;
    }

    this->execute();
}

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::force_script_state(EMonsterState state)
{
    // установить текущее состояние
    this->select_state(state);
}

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::execute_script_state()
{
    // выполнить текущее состояние
    this->get_state_current()->execute();
}

TEMPLATE_SPECIALIZATION
bool CMonsterStateManagerAbstract::can_eat()
{
    if (!this->object->CorpseMan.get_corpse())
        return false;

    return check_state(eStateEat);
}

TEMPLATE_SPECIALIZATION
bool CMonsterStateManagerAbstract::check_state(u32 state_id)
{
    if (this->prev_substate == state_id)
    {
        if (!this->get_state_current()->check_completion())
            return true;
    }
    else
    {
        if (this->get_state(state_id)->check_start_conditions())
            return true;
    }

    return false;
}

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::critical_finalize() { inherited::critical_finalize(); }
TEMPLATE_SPECIALIZATION
EMonsterState CMonsterStateManagerAbstract::get_state_type() { return inherited::get_state_type(); }
#ifdef DEBUG

TEMPLATE_SPECIALIZATION
void CMonsterStateManagerAbstract::add_debug_info(debug::text_tree& root_s) { CState<_Object>::add_debug_info(root_s); }
#endif

#undef CMonsterStateManagerAbstract
#undef TEMPLATE_SPECIALIZATION
