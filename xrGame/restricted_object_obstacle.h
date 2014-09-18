////////////////////////////////////////////////////////////////////////////
//	Module 		: restricted_object_obstacle.h
//	Created 	: 18.08.2004
//  Modified 	: 23.08.2004
//	Author		: Dmitriy Iassenev
//	Description : Restricted object with obstacles' avoidance
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "restricted_object.h"
#include <boost/noncopyable.hpp>

class obstacles_query;

class CRestrictedObjectObstacle : public CRestrictedObject, private boost::noncopyable {
private:
	typedef CRestrictedObject	inherited;

private:
	const obstacles_query		&m_static_query;
	const obstacles_query		&m_dynamic_query;

private:
			void				apply						(const obstacles_query &query, const u32 &start_vertex_id) const;
			void				apply						(const obstacles_query &query, const Fvector &start_position, const Fvector &dest_position) const;
			void				apply						(const obstacles_query &query, const u32 &start_vertex_id, const u32 &dest_vertex_id) const;

public:
								CRestrictedObjectObstacle	(CCustomMonster *object, const obstacles_query &static_query, const obstacles_query &dynamic_query);
	virtual	void				add_border					(u32 start_vertex_id, float radius) const;
	virtual	void				add_border					(const Fvector &start_position, const Fvector &dest_position) const;
	virtual	void				add_border					(u32 start_vertex_id, u32 dest_vertex_id) const;
	virtual	void				remove_border				() const;

	IC	const obstacles_query	&static_query				() const
	{
		VERIFY					(!applied());
		return					(m_static_query);
	}

	IC	const obstacles_query	&dynamic_query				() const
	{
		VERIFY					(!applied());
		return					(m_dynamic_query);
	}
};
