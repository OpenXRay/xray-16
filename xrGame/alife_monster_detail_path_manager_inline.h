////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_monster_detail_path_manager_inline.h
//	Created 	: 01.11.2005
//  Modified 	: 22.11.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife monster detail path manager class inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CALifeMonsterDetailPathManager::object_type &CALifeMonsterDetailPathManager::object	() const
{
	VERIFY		(m_object);
	return		(*m_object);
}

IC	void CALifeMonsterDetailPathManager::speed										(const float &speed)
{
	VERIFY		(_valid(speed));
	m_speed		= speed;
}

IC	const float &CALifeMonsterDetailPathManager::speed								() const
{
	VERIFY		(_valid(m_speed));
	return		(m_speed);
}

IC	const CALifeMonsterDetailPathManager::PATH &CALifeMonsterDetailPathManager::path	() const
{
	return		(m_path);
}

IC	const float	&CALifeMonsterDetailPathManager::walked_distance					() const
{
	VERIFY		(path().size() > 1);
	return		(m_walked_distance);
}
