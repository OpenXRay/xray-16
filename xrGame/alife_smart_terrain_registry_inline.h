////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_smart_terrain_registry_inline.h
//	Created 	: 20.09.2005
//  Modified 	: 20.09.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife smart terrain registry inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	const CALifeSmartTerrainRegistry::OBJECTS &CALifeSmartTerrainRegistry::objects	() const
{
	return					(m_objects);
}

IC	CSE_ALifeSmartZone *CALifeSmartTerrainRegistry::object							(const ALife::_OBJECT_ID &id) const
{
	OBJECTS::const_iterator	I = objects().find(id);
	VERIFY					(I != objects().end());
	return					((*I).second);
}
