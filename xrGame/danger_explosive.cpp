////////////////////////////////////////////////////////////////////////////
//	Module 		: danger_explosive.cpp
//	Created 	: 08.06.2005
//  Modified 	: 08.06.2005
//	Author		: Dmitriy Iassenev
//	Description : Danger explosive class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "danger_explosive.h"
#include "gameobject.h"
#include "explosive.h"

bool CDangerExplosive::operator==	(const u16 &object_id) const
{
	if (!m_grenade)
		return	(false);
	
	const CGameObject	*game_object = smart_cast<const CGameObject*>(m_grenade);
	VERIFY				(game_object);
	return				(game_object->ID() == object_id);
}
