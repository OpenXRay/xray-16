////////////////////////////////////////////////////////////////////////////
//	Module 		: hit_memory_manager_inline.h
//	Created 	: 25.12.2003
//  Modified 	: 25.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Hit memory manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CHitMemoryManager::CHitMemoryManager							(CCustomMonster *object, CAI_Stalker *stalker)
{
	VERIFY			(object);
	m_object		= object;
	m_stalker		= stalker;
#ifdef USE_SELECTED_HIT
	m_selected_hit	= 0;
#endif
}

IC	const CHitMemoryManager::HITS &CHitMemoryManager::objects		() const
{
	VERIFY			(m_hits);
	return			(*m_hits);
}

#ifdef USE_SELECTED_HIT
IC	const CHitMemoryManager::CHitObject *CHitMemoryManager::hit		() const
{
	return			(m_selected_hit);
}
#endif

IC	void CHitMemoryManager::set_squad_objects						(xr_vector<CHitObject> *squad_objects)
{
	m_hits			= squad_objects;
}

IC	CCustomMonster &CHitMemoryManager::object						() const
{
	VERIFY			(m_object);
	return			(*m_object);
}

IC const ALife::_OBJECT_ID &CHitMemoryManager::last_hit_object_id	() const
{
	return			(m_last_hit_object_id);
}

IC	u32 const& CHitMemoryManager::last_hit_time						() const
{
	return			(m_last_hit_time);
}
