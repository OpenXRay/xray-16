////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_manager_inline.h
//	Created 	: 06.11.2007
//  Modified 	: 06.11.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover manager class
////////////////////////////////////////////////////////////////////////////

#if 0//ndef SMART_COVER_MANAGER_INLINE_H_INCLUDED
#define SMART_COVER_MANAGER_INLINE_H_INCLUDED

#define Manager	smart_cover::manager

IC	smart_cover::cover const *Manager::current_cover				() const
{
	return					(m_current_cover);
}

IC	smart_cover::loophole const *Manager::current_loophole			() const
{
	return					(m_current_loophole);
}

IC	smart_cover::loophole const *Manager::target_loophole			() const
{
	return					(m_target_loophole);
}

IC	CAI_Stalker &Manager::object									() const
{
	VERIFY					(m_object);
	return					(*m_object);
}

IC	shared_str const &Manager::cover_id								() const
{
	return					(m_cover_id);
}

IC	void Manager::cover_id											(shared_str const &id)
{
	m_cover_id				= id;
}

IC	bool const &Manager::default_behaviour							() const
{
	return					(m_default_behaviour);
}

IC	void Manager::default_behaviour									(bool value)
{
	m_default_behaviour		= value;
}

IC	Fvector const &Manager::fire_position							() const
{
	return					(m_fire_position);
}

IC	void Manager::fire_position										(Fvector const &value)
{
	m_fire_position			= value;
}

IC	void Manager::invalidate_fire_position							()
{
	m_fire_position.set		(ms_invalid_position);
}

IC	bool Manager::has_fire_position									() const
{
	return					(m_fire_position.similar(Manager::ms_invalid_position) == FALSE);
}

#undef Manager

#endif // SMART_COVER_MANAGER_INLINE_H_INCLUDED