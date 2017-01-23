////////////////////////////////////////////////////////////////////////////
//	Module 		: setup_manager_inline.h
//	Created 	: 05.04.2004
//  Modified 	: 05.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Setup manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
	template <\
		typename _action_type,\
		typename _object_type,\
		typename _action_id_type\
	>

#define CSSetupManager CSetupManager<_action_type,_object_type,_action_id_type>

TEMPLATE_SPECIALIZATION
IC	CSSetupManager::CSetupManager					(_object_type *object)
{
	VERIFY					(object);
	m_object				= object;
}

TEMPLATE_SPECIALIZATION
CSSetupManager::~CSetupManager						()
{
	clear					();
}

TEMPLATE_SPECIALIZATION
void CSSetupManager::reinit							()
{
	clear					();
	m_actuality				= false;
}

TEMPLATE_SPECIALIZATION
IC	_action_type &CSSetupManager::action			(const _action_id_type &action_id) const
{
	setup_actions::const_iterator	I = std::find_if(actions().begin(),actions().end(),setup_pred(action_id));
	VERIFY					(I != actions().end());
	return					(*(*I).second);
}

TEMPLATE_SPECIALIZATION
IC	_action_type &CSSetupManager::current_action	() const
{
	return					(action(current_action_id()));
}

TEMPLATE_SPECIALIZATION
IC	const _action_id_type &CSSetupManager::current_action_id	() const
{
	return					(m_current_action_id);
}

TEMPLATE_SPECIALIZATION
IC	void CSSetupManager::clear						()
{
	m_actuality				= false;
	m_current_action_id		= _action_id_type(-1);
	m_previous_action_id	= _action_id_type(-1);
	delete_data				(actions());
}

TEMPLATE_SPECIALIZATION
IC	void CSSetupManager::add_action					(const _action_id_type &action_id, _action_type *action)
{
	m_actuality				= false;
	VERIFY					(action);
	VERIFY					(std::find_if(actions().begin(),actions().end(),setup_pred(action_id)) == actions().end());
	action->set_object		(m_object);
	if (actions().empty())
		m_current_action_id	= action_id;
	actions().push_back		(std::make_pair(action_id,action));
}

TEMPLATE_SPECIALIZATION
void CSSetupManager::update							()
{
	if (actions().empty())
		return;
	
	select_action			();
	
	if (m_previous_action_id != current_action_id())
		current_action().initialize();

	m_previous_action_id	= current_action_id();

	current_action().execute();
}

TEMPLATE_SPECIALIZATION
IC	void CSSetupManager::select_action				()
{
	if (!m_actuality || current_action().completed()) {
		m_actuality			= true;
		if (actions().size() == 1) {
			if (m_current_action_id != (*actions().begin()).first)
				(*actions().begin()).second->initialize();

			m_current_action_id = (*actions().begin()).first;
			return;
		}

		float				m_total_weight = 0.f;
		setup_actions::const_iterator	I = actions().begin();
		setup_actions::const_iterator	E = actions().end();
		for ( ; I != E; ++I)
			if (((*I).first != m_current_action_id) && (*I).second->applicable())
				m_total_weight += (*I).second->weight();
		VERIFY				(!fis_zero(m_total_weight));

		float				m_random = ::Random.randF(m_total_weight);
		m_total_weight		= 0.f;
		I					= actions().begin();
		for ( ; I != E; ++I) {
			if (((*I).first != m_current_action_id) && (*I).second->applicable())
				m_total_weight += (*I).second->weight();
			else
				continue;
			if (m_total_weight > m_random) {
				if (std::find_if(actions().begin(),actions().end(),setup_pred(m_current_action_id)) != actions().end())
					current_action().finalize();
				m_current_action_id = (*I).first;
				(*I).second->initialize();
				break;
			}
		}
	}
}

TEMPLATE_SPECIALIZATION
IC	_object_type &CSSetupManager::object	() const
{
	VERIFY		(m_object);
	return		(*m_object);
}

TEMPLATE_SPECIALIZATION
IC	const typename CSSetupManager::setup_actions &CSSetupManager::actions	() const
{
	return		(m_actions);
}

TEMPLATE_SPECIALIZATION
IC	typename CSSetupManager::setup_actions &CSSetupManager::actions	()
{
	return		(m_actions);
}

#undef TEMPLATE_SPECIALIZATION
#undef CSSetupManager