#include "StdAfx.h"
#include "event_conditions_collection.h"
#include "Common/object_broker.h"
#include "game_state_accumulator.h"
#include "Level.h"
#include "game_cl_base.h"

namespace award_system
{
event_conditions_collection::event_conditions_collection(
    game_state_accumulator* pstate_accum, event_action_delegate_t ea_delegate)
    : m_player_state_accum(pstate_accum), m_event_action(ea_delegate)
{
}

event_conditions_collection::~event_conditions_collection()
{
    m_root_conditions.clear();
    delete_data(m_all_conditions);
}

void event_conditions_collection::clear_events()
{
    m_root_conditions.clear();
    delete_data(m_all_conditions);
}

void event_conditions_collection::check_for_events()
{
    // std::foreach
    for (event_root_conditions_t::iterator i = m_root_conditions.begin(), ie = m_root_conditions.end(); i != ie; ++i)
    {
        execute_root_condtiion(*i);
    }
}

event_condition_t* event_conditions_collection::add_condition(
    enum_event_operation operation, buffer_vector<event_argument_type>& arguments)
{
    event_condition_t* tmp_condition = new event_condition_t();
    tmp_condition->m_operation = operation;

    for (buffer_vector<event_argument_type>::const_iterator i = arguments.begin(), ie = arguments.end(); i != ie; ++i)
    {
        tmp_condition->m_arguments.push_back(*i);
    }

    m_all_conditions.push_back(tmp_condition);
    return tmp_condition;
}

void event_conditions_collection::add_event(
    event_condition_t* root_condition, u32 const max_rise_count, u32 const game_id_mask, u32 const delegate_argument)
{
    event_root_condition_t tmp_root_condition;
    tmp_root_condition.m_delegate_argument = delegate_argument;
    tmp_root_condition.m_rise_count = max_rise_count;
    tmp_root_condition.m_game_mask = game_id_mask;
    tmp_root_condition.m_root_condition = root_condition;
    m_root_conditions.push_back(tmp_root_condition);
}

//-----executing condiditons ----

bool event_conditions_collection::logical_and(arguments_t& arguments)
{
    VERIFY(arguments.size() >= 2);
    bool result = true;
    for (arguments_t::iterator i = arguments.begin(), ie = arguments.end(); i != ie; ++i)
    {
        VERIFY(i->m_argument_type_tag == event_argument_type::at_condition);
        result &= execute_condition(i->m_argument_value.cond_ptr_value);
        if (!result)
            return false;
    }
    return true;
}

bool event_conditions_collection::logical_or(arguments_t& arguments)
{
    VERIFY(arguments.size() >= 2);
    bool result = false;
    for (arguments_t::iterator i = arguments.begin(), ie = arguments.end(); i != ie; ++i)
    {
        VERIFY(i->m_argument_type_tag == event_argument_type::at_condition);
        result |= execute_condition(i->m_argument_value.cond_ptr_value);
        if (result)
            return true;
    }
    return false;
}

enum enum_hit_params_args
{
    hpa_hit_count = 0x00,
    hpa_hit_wpn_group_id,
    hpa_hit_bone_group_id,
    hpa_bfunc,
    hpa_hit_distance,
    hpa_args_count
}; // enum enum_hit_params_args

bool event_conditions_collection::hit_params(arguments_t& arguments)
{
    VERIFY(arguments.size() == hpa_args_count);
    VERIFY(arguments[hpa_hit_count].m_argument_type_tag == event_argument_type::at_u32);
    VERIFY(arguments[hpa_hit_wpn_group_id].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY(arguments[hpa_hit_bone_group_id].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY(arguments[hpa_bfunc].m_argument_type_tag == event_argument_type::at_float_bfunction);
    VERIFY(arguments[hpa_hit_distance].m_argument_type_tag == event_argument_type::at_float);

    return m_player_state_accum->check_hit_params(arguments[hpa_hit_count].m_argument_value.u32_value,
        static_cast<ammunition_group::enum_group_id>(arguments[hpa_hit_wpn_group_id].m_argument_value.u16_value),
        static_cast<bone_group::enum_group_id>(arguments[hpa_hit_bone_group_id].m_argument_value.u16_value),
        arguments[hpa_bfunc].m_argument_value.float_function_ptr,
        arguments[hpa_hit_distance].m_argument_value.float_value);
}

enum enum_kill_params_args
{
    kpa_kill_count = 0x00,
    kpa_kill_wpn_group_id,
    kpa_kill_kill_type,
    kpa_kill_spec_kill_type,
    kpa_kill_time_period,
    kpa_args_count
}; // enum enum_kill_params_args

bool event_conditions_collection::kill_params(arguments_t& arguments)
{
    VERIFY(arguments.size() == kpa_args_count);
    VERIFY(arguments[kpa_kill_count].m_argument_type_tag == event_argument_type::at_u32);
    VERIFY(arguments[kpa_kill_wpn_group_id].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY(arguments[kpa_kill_kill_type].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY(arguments[kpa_kill_spec_kill_type].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY(arguments[kpa_kill_time_period].m_argument_type_tag == event_argument_type::at_u32);

    return m_player_state_accum->check_kill_params(arguments[kpa_kill_count].m_argument_value.u32_value,
        static_cast<ammunition_group::enum_group_id>(arguments[kpa_kill_wpn_group_id].m_argument_value.u16_value),
        static_cast<KILL_TYPE>(arguments[kpa_kill_kill_type].m_argument_value.u16_value),
        static_cast<SPECIAL_KILL_TYPE>(arguments[kpa_kill_spec_kill_type].m_argument_value.u16_value),
        arguments[kpa_kill_time_period].m_argument_value.u32_value);
}

enum enum_accumul_params_args
{
    cpa_param_id,
    cpa_bfunc,
    cpa_value,
    cpa_args_count
}; // enum enum_accumul_params_args

bool event_conditions_collection::accumul_params(arguments_t& arguments)
{
    VERIFY(arguments.size() == cpa_args_count);
    VERIFY(arguments[cpa_param_id].m_argument_type_tag == event_argument_type::at_u16);
    VERIFY((arguments[cpa_bfunc].m_argument_type_tag == event_argument_type::at_float_bfunction) ||
        (arguments[cpa_bfunc].m_argument_type_tag == event_argument_type::at_u32_bfunction));
    VERIFY((arguments[cpa_value].m_argument_type_tag == event_argument_type::at_float) ||
        (arguments[cpa_value].m_argument_type_tag == event_argument_type::at_u32));
    if (arguments[cpa_value].m_argument_type_tag == event_argument_type::at_float)
    {
        return m_player_state_accum->check_accumulative_value(
            static_cast<enum_accumulative_player_values>(arguments[cpa_param_id].m_argument_value.u16_value),
            arguments[cpa_bfunc].m_argument_value.float_function_ptr,
            arguments[cpa_value].m_argument_value.float_value);
    }
    return m_player_state_accum->check_accumulative_value(
        static_cast<enum_accumulative_player_values>(arguments[cpa_param_id].m_argument_value.u16_value),
        arguments[cpa_bfunc].m_argument_value.u32_function_ptr, arguments[cpa_value].m_argument_value.u32_value);
}

bool event_conditions_collection::execute_condition(event_condition_t* cond)
{
    VERIFY(cond);
    bool result = false;
    switch (cond->m_operation)
    {
    case eo_logical_and: result = logical_and(cond->m_arguments); break;
    case eo_logical_or: result = logical_or(cond->m_arguments); break;
    case eo_hit_params: result = hit_params(cond->m_arguments); break;
    case eo_kill_params: result = kill_params(cond->m_arguments); break;
    case eo_accumul_value_params: result = accumul_params(cond->m_arguments); break;
    }; // switch (cond->m_operation)
    return result;
}

void event_conditions_collection::execute_root_condtiion(event_root_conditions_t::value_type& rcond)
{
    if (!rcond.m_rise_count)
        return;

    if ((rcond.m_game_mask & Game().Type()) == 0)
        return;

    VERIFY(rcond.m_root_condition);
    if (execute_condition(rcond.m_root_condition))
    {
        m_event_action(rcond.m_delegate_argument);
        --rcond.m_rise_count;
    }
}

//---helper functions ----

event_condition_t* event_conditions_collection::add_and_condition(event_condition_t* left, event_condition_t* right)
{
    buffer_vector<event_argument_type> args_buffer(_alloca(sizeof(event_argument_type) * 2), 2);
    event_argument_type tmp_arg;
    tmp_arg.m_argument_type_tag = event_argument_type::at_condition;
    tmp_arg.m_argument_value.cond_ptr_value = left;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_value.cond_ptr_value = right;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_logical_and, args_buffer);
}

event_condition_t* event_conditions_collection::add_or_condition(event_condition_t* left, event_condition_t* right)
{
    buffer_vector<event_argument_type> args_buffer(_alloca(sizeof(event_argument_type) * 2), 2);
    event_argument_type tmp_arg;
    tmp_arg.m_argument_type_tag = event_argument_type::at_condition;
    tmp_arg.m_argument_value.cond_ptr_value = left;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_value.cond_ptr_value = right;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_logical_or, args_buffer);
}

event_condition_t* event_conditions_collection::add_hit_condition_dist(
    u32 hit_counts, u16 weapon_id, u16 bone_id, float_binary_function* fbfunc, float distanse)
{
    buffer_vector<event_argument_type> args_buffer(
        _alloca(sizeof(event_argument_type) * hpa_args_count), hpa_args_count);
    event_argument_type tmp_arg;
    tmp_arg.m_argument_type_tag = event_argument_type::at_u32;
    tmp_arg.m_argument_value.u32_value = hit_counts;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = weapon_id;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = bone_id;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_float_bfunction;
    tmp_arg.m_argument_value.float_function_ptr = fbfunc;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_float;
    tmp_arg.m_argument_value.float_value = distanse;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_hit_params, args_buffer);
}

event_condition_t* event_conditions_collection::add_kill_condition_dist(
    u32 kill_counts, u16 weapon_id, u16 kill_type, u16 special_kill_type, u32 time_period)
{
    buffer_vector<event_argument_type> args_buffer(
        _alloca(sizeof(event_argument_type) * kpa_args_count), kpa_args_count);
    event_argument_type tmp_arg;
    tmp_arg.m_argument_type_tag = event_argument_type::at_u32;
    tmp_arg.m_argument_value.u32_value = kill_counts;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = weapon_id;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = kill_type;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = special_kill_type;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u32;
    tmp_arg.m_argument_value.u32_value = time_period;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_kill_params, args_buffer);
}

event_condition_t* event_conditions_collection::add_accumm_value_condition(
    u16 param_id, float_binary_function* fbfunc, float argument)
{
    buffer_vector<event_argument_type> args_buffer(
        _alloca(sizeof(event_argument_type) * cpa_args_count), cpa_args_count);
    event_argument_type tmp_arg;

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = param_id;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_float_bfunction;
    tmp_arg.m_argument_value.float_function_ptr = fbfunc;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_float;
    tmp_arg.m_argument_value.float_value = argument;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_accumul_value_params, args_buffer);
}

event_condition_t* event_conditions_collection::add_accumm_value_condition(
    u16 param_id, u32_binary_function* fbfunc, u32 argument)
{
    buffer_vector<event_argument_type> args_buffer(
        _alloca(sizeof(event_argument_type) * cpa_args_count), cpa_args_count);
    event_argument_type tmp_arg;

    tmp_arg.m_argument_type_tag = event_argument_type::at_u16;
    tmp_arg.m_argument_value.u16_value = param_id;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u32_bfunction;
    tmp_arg.m_argument_value.u32_function_ptr = fbfunc;
    args_buffer.push_back(tmp_arg);

    tmp_arg.m_argument_type_tag = event_argument_type::at_u32;
    tmp_arg.m_argument_value.u32_value = argument;
    args_buffer.push_back(tmp_arg);

    return add_condition(eo_accumul_value_params, args_buffer);
}

} // namespace award_system
