////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects_inline.h
//	Created 	: 28.03.2007
//  Modified 	: 28.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving object inline functions
////////////////////////////////////////////////////////////////////////////

#ifndef MOVING_OBJECT_INLINE_H
#define MOVING_OBJECT_INLINE_H

IC	shared_str moving_object::id							() const
{
	VERIFY				(m_object);
	return				(m_object->cName());
}

IC	const Fvector &moving_object::position					() const
{
	return				(m_position);
}

IC	float moving_object::radius								() const
{
	return				(m_object->Radius());
}

IC	const CEntityAlive &moving_object::object				() const
{
	VERIFY				(m_object);
	return				(*m_object);
}

IC	void moving_object::action								(const action_type &action)
{
	m_action_frame		= Device.dwFrame;
	VERIFY				((action == moving_object::action_move) || (action == moving_object::action_wait));
	if (action == m_action)
		return;

	m_action			= action;
	m_action_position	= Fvector().set(flt_max,flt_max,flt_max);
	m_action_time		= Device.dwTimeGlobal;
#if 0//def DEBUG
	Msg					("%6d %s %s",Device.dwFrame,*object().cName(),action == moving_object::action_wait ? "wait" : "move");
#endif // DEBUG
}

IC	void moving_object::action								(const action_type &action, const Fvector &action_position)
{
	m_action_frame		= Device.dwFrame;
	VERIFY				((action == moving_object::action_move) || (action == moving_object::action_wait));
	if (action == m_action)
		return;

	m_action			= action;
	m_action_position	= action_position;
	m_action_time		= Device.dwTimeGlobal;
#if 0//def DEBUG
	Msg					("%6d %s %s",Device.dwFrame,*object().cName(),action == moving_object::action_wait ? "wait" : "move");
#endif // DEBUG
}

IC	const moving_object::action_type &moving_object::action	() const
{
	return				(m_action);
}

IC	const Fvector &moving_object::action_position			() const
{
	return				(m_action_position);
}

IC	const u32 &moving_object::action_frame					() const
{
	return				(m_action_frame);
}

IC	const u32 &moving_object::action_time					() const
{
	return				(m_action_time);
}

IC	obstacles_query &moving_object::static_query			()
{
	return				(m_static_query);
}

IC	obstacles_query &moving_object::dynamic_query			()
{
	return				(m_dynamic_query);
}

#endif // MOVING_OBJECT_INLINE_H