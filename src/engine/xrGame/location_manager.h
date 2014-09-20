////////////////////////////////////////////////////////////////////////////
//	Module 		: location_manager.h
//	Created 	: 27.12.2003
//  Modified 	: 27.12.2003
//	Author		: Dmitriy Iassenev
//	Description : Location manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "game_graph_space.h"

class CGameObject;

class CLocationManager {
private:
	GameGraph::TERRAIN_VECTOR	m_vertex_types;
	CGameObject					*m_object;

public:
	IC				CLocationManager			(CGameObject *object);
	virtual			~CLocationManager			();
	virtual void	Load						(LPCSTR section);
	virtual void	reload						(LPCSTR section);
	void			clear_location_types		();
	void			add_location_type			(LPCSTR mask);
	IC		const GameGraph::TERRAIN_VECTOR &vertex_types	() const;
};

#include "location_manager_inline.h"
