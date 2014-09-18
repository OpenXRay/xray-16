////////////////////////////////////////////////////////////////////////////
//	Module 		: space_restriction_abstract.h
//	Created 	: 10.08.2005
//  Modified 	: 10.08.2005
//	Author		: Dmitriy Iassenev
//	Description : Space restriction abstract
////////////////////////////////////////////////////////////////////////////

#pragma once

class CSpaceRestrictionAbstract {
protected:
	xr_vector<u32>					m_border;
	bool							m_initialized;
	xr_vector<u32>					m_accessible_neighbour_border;
	bool							m_accessible_neighbour_border_actual;

private:
	template <typename T>
	IC		bool					accessible_neighbours				(T &restriction, u32 level_vertex_id, bool out_restriction);
	
	template <typename T>
	IC		void					prepare_accessible_neighbour_border	(T &restriction, bool out_restriction);

public:
	IC								CSpaceRestrictionAbstract			();
	virtual							~CSpaceRestrictionAbstract			() {}
	virtual	void					initialize							() = 0;
	IC		const xr_vector<u32>	&border								();
	IC		bool					initialized							() const;

public:
	template <typename T>
	IC		const xr_vector<u32>	&accessible_neighbour_border		(T &restriction, bool out_restriction);

public:
	virtual shared_str				name								() const = 0;
};

#include "space_restriction_abstract_inline.h"