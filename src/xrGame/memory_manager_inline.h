////////////////////////////////////////////////////////////////////////////
//	Module 		: memory_manager_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 19.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Memory manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

template <typename T, typename _predicate>
IC	void CMemoryManager::fill_enemies	(const xr_vector<T> &objects, const _predicate &predicate) const
{
	xr_vector<T>::const_iterator	I = objects.begin();
	xr_vector<T>::const_iterator	E = objects.end();
	for ( ; I != E; ++I) {
		if (!(*I).m_enabled)
			continue;

		const CEntityAlive	*_enemy = smart_cast<const CEntityAlive*>((*I).m_object);

		if (_enemy && enemy().useful(_enemy))
			predicate		(_enemy);
	}
}

template <typename _predicate>
IC	void CMemoryManager::fill_enemies	(const _predicate &predicate) const
{
	fill_enemies			(visual().objects(),predicate);
//	fill_enemies			(sound().objects(),predicate);
//	fill_enemies			(hit().objects(),predicate);
}

IC	CVisualMemoryManager	&CMemoryManager::visual		() const
{
	VERIFY					(m_visual);
	return					(*m_visual);
}

IC	CSoundMemoryManager		&CMemoryManager::sound		() const
{
	VERIFY					(m_sound);
	return					(*m_sound);
}

IC	CHitMemoryManager		&CMemoryManager::hit		() const
{
	VERIFY					(m_hit);
	return					(*m_hit);
}

IC	CEnemyManager			&CMemoryManager::enemy		() const
{
	VERIFY					(m_enemy);
	return					(*m_enemy);
}

IC	CItemManager			&CMemoryManager::item		() const
{
	VERIFY					(m_item);
	return					(*m_item);
}

IC	CDangerManager			&CMemoryManager::danger		() const
{
	VERIFY					(m_danger);
	return					(*m_danger);
}

IC	CCustomMonster &CMemoryManager::object				() const
{
	VERIFY					(m_object);
	return					(*m_object);
}

IC	CAI_Stalker	&CMemoryManager::stalker				() const
{
	VERIFY					(m_stalker);
	return					(*m_stalker);
}