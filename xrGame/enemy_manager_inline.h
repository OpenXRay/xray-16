////////////////////////////////////////////////////////////////////////////
//	Module 		: enemy_manager_inline.h
//	Created 	: 30.12.2003
//  Modified 	: 30.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Enemy manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	u32	CEnemyManager::last_enemy_time								() const
{
	return						(m_last_enemy_time);
}

IC	const CEntityAlive *CEnemyManager::last_enemy					() const
{
	return						(m_last_enemy);
}

IC	CEnemyManager::USEFULE_CALLBACK &CEnemyManager::useful_callback	()
{
	return						(m_useful_callback);
}

IC	void CEnemyManager::enable_enemy_change							(const bool &value)
{
	m_enable_enemy_change		= value;
}

IC	bool CEnemyManager::enable_enemy_change							() const
{
	return						(m_enable_enemy_change);
}

IC	CEntityAlive const *CEnemyManager::selected						() const
{
	if (m_smart_cover_enemy && m_smart_cover_enemy->g_Alive())
		return					(m_smart_cover_enemy);

	return						(inherited::selected());
}

IC	void CEnemyManager::set_enemy									(CEntityAlive const	*enemy)
{
	VERIFY2						(enemy, "Bad enemy!");
	VERIFY2						(enemy->g_Alive(), "Enemy is already dead!");
	
	m_smart_cover_enemy			= enemy;
}

IC	void CEnemyManager::invalidate_enemy							()
{
	m_smart_cover_enemy			= 0;
}