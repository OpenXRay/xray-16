////////////////////////////////////////////////////////////////////////////
//	Module 		: member_enemy_inline.h
//	Created 	: 24.05.2004
//  Modified 	: 14.01.2005
//	Author		: Dmitriy Iassenev
//	Description : Member enemy inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CMemberEnemy::CMemberEnemy		(const CEntityAlive *object, squad_mask_type mask)
{
	m_object					= object;
	m_mask.assign				(mask);
	m_probability				= 1.f;
	m_distribute_mask.zero		();
}

IC	bool CMemberEnemy::operator==	(const CEntityAlive *object) const
{
	return						(m_object == object);
}

IC	bool CMemberEnemy::operator<	(const CMemberEnemy &enemy) const
{
	return						(m_probability > enemy.m_probability);
}
