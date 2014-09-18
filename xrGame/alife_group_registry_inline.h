////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_group_registry_inline.h
//	Created 	: 28.10.2005
//  Modified 	: 28.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife group registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const CALifeGroupRegistry::OBJECTS &CALifeGroupRegistry::objects	() const
{
	return		(m_objects);
}
