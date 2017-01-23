////////////////////////////////////////////////////////////////////////////
//	Module 		: location_manager_inline.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Location manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

IC	CLocationManager::CLocationManager								(CGameObject *object)
{
	VERIFY							(object);
	m_object						= object;
	m_vertex_types.clear			();
}

IC	const GameGraph::TERRAIN_VECTOR	&CLocationManager::vertex_types	() const
{
	return					(m_vertex_types);
}
