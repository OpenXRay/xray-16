////////////////////////////////////////////////////////////////////////////
//	Module 		: obstacles_query.h
//	Created 	: 10.04.2007
//  Modified 	: 10.04.2007
//	Author		: Dmitriy Iassenev
//	Description : obstacles query
////////////////////////////////////////////////////////////////////////////

#ifndef OBSTACLES_QUERY_H
#define OBSTACLES_QUERY_H

#include "object_broker.h"
#include "associative_vector.h"

class CGameObject;

class obstacles_query {
public:
	typedef xr_vector<u32>						AREA;
	typedef associative_vector<
				const CGameObject*,
				u32
			>									OBSTACLES;

private:
	AREA					m_area;
	OBSTACLES				m_obstacles;
	u32						m_crc;
	bool					m_actual;

private:
	IC		void			init				();
			void			merge				(const AREA &object_area);
			void			compute_area		();
							obstacles_query		(const obstacles_query &);
			obstacles_query &operator=			(const obstacles_query &);

public:
	IC						obstacles_query		();
	IC		void			clear				();
	IC		void			swap				(obstacles_query &object);
	IC		void			copy				(const obstacles_query &object);
	IC		void			add					(const CGameObject *object);
	IC		bool			refresh_objects		();
			void			set_intersection	(const obstacles_query &query);
			void			merge				(const obstacles_query &query);
			bool			merge				(const Fvector &position, const float &radius, const obstacles_query &query);
			bool			objects_changed		(const Fvector &position, const float &radius) const;
			bool			remove_objects		(const Fvector &position, const float &radius);
	IC		bool			update_objects		(const Fvector &position, const float &radius);
			void			remove_links		(CObject *object);

public:
	IC		bool			operator==			(const obstacles_query &object) const;
	IC		bool			operator!=			(const obstacles_query &object) const;
	IC		AREA			&area				();
	IC		const AREA		&area				() const;
	IC		const OBSTACLES	&obstacles			() const;
	IC		const u32		&crc				() const;
	IC		const bool		&actual				() const;
};

#include "obstacles_query_inline.h"

#endif // OBSTACLES_QUERY_H