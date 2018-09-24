////////////////////////////////////////////////////////////////////////////
//	Module 		: action_base_inline.h
//	Created 	: 28.01.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Base action inline function
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Level.h"
#include "xrEngine/device.h"

#define TEMPLATE_SPECIALIZATION template <typename _object_type>
#define CBaseAction CActionBase<_object_type>

TEMPLATE_SPECIALIZATION
IC CBaseAction::CActionBase(_object_type* object, LPCSTR action_name) { init(object, action_name); }
TEMPLATE_SPECIALIZATION
CBaseAction::~CActionBase() {}
TEMPLATE_SPECIALIZATION
void CBaseAction::init(_object_type* object, LPCSTR action_name)
{
    m_storage = 0;
    m_object = object;
    m_weight = edge_value_type(1);

#ifdef LOG_ACTION
    m_use_log = false;
    m_action_name = action_name;
    m_switched = false;
//	if (xr_strlen(m_action_name))
//		debug_log		(eActionStateConstructed);
#endif
}

TEMPLATE_SPECIALIZATION
void CBaseAction::setup(_object_type* object, CPropertyStorage* storage)
{
    VERIFY(object);
    VERIFY(storage);
    m_object = object;
    m_storage = storage;
    m_inertia_time = 0;
#ifdef LOG_ACTION
    m_switched = false;
    if (m_use_log && xr_strlen(m_action_name))
        debug_log(eActionStateSetup);
#endif
}

TEMPLATE_SPECIALIZATION
void CBaseAction::initialize()
{
#ifdef LOG_ACTION
    VERIFY3(!m_switched, m_action_name, "::initialize()");
    m_switched = true;
    if (m_use_log && xr_strlen(m_action_name))
        debug_log(eActionStateInitialized);
#endif
    m_start_level_time = Device.dwTimeGlobal;
    m_first_time = true;
}

TEMPLATE_SPECIALIZATION
void CBaseAction::execute()
{
    m_first_time = false;
#ifdef LOG_ACTION
    if (m_use_log && xr_strlen(m_action_name) && m_switched)
        debug_log(eActionStateExecuted);
    m_switched = false;
#endif
}

TEMPLATE_SPECIALIZATION
void CBaseAction::finalize()
{
#ifdef LOG_ACTION
    VERIFY3(!m_switched, m_action_name, "::finalize()");
    if (m_use_log && xr_strlen(m_action_name))
        debug_log(eActionStateFinalized);
#endif
}

TEMPLATE_SPECIALIZATION
bool CBaseAction::completed() const { return (m_start_level_time + m_inertia_time <= Device.dwTimeGlobal); }
TEMPLATE_SPECIALIZATION
IC u32 CBaseAction::start_level_time() const { return (m_start_level_time); }
TEMPLATE_SPECIALIZATION
IC u32 CBaseAction::inertia_time() const { return (m_inertia_time); }
TEMPLATE_SPECIALIZATION
IC void CBaseAction::set_inertia_time(u32 inertia_time) { m_inertia_time = inertia_time; }
#ifdef LOG_ACTION
TEMPLATE_SPECIALIZATION
IC void CBaseAction::debug_log(const EActionStates state_state) const
{
    switch (state_state)
    {
    case eActionStateConstructed:
    {
        Msg("[%6d] action %s is constructed", Device.dwTimeGlobal, m_action_name);
        break;
    }
    case eActionStateSetup:
    {
        Msg("[%6d] action %s is setup", Device.dwTimeGlobal, m_action_name);
        break;
    }
    case eActionStateInitialized:
    {
        Msg("[%6d] action %s is initialized", Device.dwTimeGlobal, m_action_name);
        break;
    }
    case eActionStateExecuted:
    {
        Msg("[%6d] action %s is executed", Device.dwTimeGlobal, m_action_name);
        break;
    }
    case eActionStateFinalized:
    {
        Msg("[%6d] action %s is finalized", Device.dwTimeGlobal, m_action_name);
        break;
    }
    default: NODEFAULT;
    }
}

TEMPLATE_SPECIALIZATION
IC void CBaseAction::set_use_log(bool value) { m_use_log = value; }
#endif

TEMPLATE_SPECIALIZATION
IC void CBaseAction::set_property(const _condition_type& condition_id, const _value_type& value)
{
    VERIFY(m_storage);
    m_storage->set_property(condition_id, value);
}

TEMPLATE_SPECIALIZATION
IC const typename CBaseAction::_value_type& CBaseAction::property(const _condition_type& condition_id) const
{
    VERIFY(m_storage);
    return (m_storage->property(condition_id));
}

TEMPLATE_SPECIALIZATION
IC void CBaseAction::set_weight(const edge_value_type& weight) { m_weight = _max(min_weight(), weight); }
TEMPLATE_SPECIALIZATION
typename CBaseAction::edge_value_type CBaseAction::weight(
    const CSConditionState& condition0, const CSConditionState& condition1) const
{
    edge_value_type _min_weight = min_weight();
    if (m_weight < _min_weight)
        m_weight = _min_weight;
    return (m_weight);
}

#ifdef LOG_ACTION
TEMPLATE_SPECIALIZATION
IC void CBaseAction::show(LPCSTR offset) {}
#endif

TEMPLATE_SPECIALIZATION
IC bool CBaseAction::first_time() const { return (m_first_time); }
#undef TEMPLATE_SPECIALIZATION
#undef CBaseAction
