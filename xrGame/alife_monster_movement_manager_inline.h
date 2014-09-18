////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_movement_manager_inline.h
//	Created 	: 31.10.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster movement manager class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeMonsterMovementManager::object_type &CALifeMonsterMovementManager::object			() const
{
	VERIFY		(m_object);
	return		(*m_object);
}

IC	CALifeMonsterMovementManager::detail_path_type &CALifeMonsterMovementManager::detail	() const
{
	VERIFY		(m_detail);
	return		(*m_detail);
}

IC	CALifeMonsterMovementManager::patrol_path_type &CALifeMonsterMovementManager::patrol	() const
{
	VERIFY		(m_patrol);
	return		(*m_patrol);
}

IC	const CALifeMonsterMovementManager::EPathType &CALifeMonsterMovementManager::path_type	() const
{
	return		(m_path_type);
}

IC	void CALifeMonsterMovementManager::path_type	(const EPathType &path_type)
{
	m_path_type	= path_type;
}
