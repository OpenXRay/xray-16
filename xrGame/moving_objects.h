////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects.h
//	Created 	: 27.03.2007
//  Modified 	: 27.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects
////////////////////////////////////////////////////////////////////////////

#ifndef MOVING_OBJECTS_H
#define MOVING_OBJECTS_H

#include "quadtree.h"
#include "obstacles_query.h"

class moving_object;
class CObject;
class MagicBox3;
struct boxes;

class moving_objects {
private:
	typedef CQuadTree<moving_object>					TREE;
	typedef xr_vector<CObject*>							NEAREST_STATIC;

public:
	enum possible_actions {
		possible_action_1_can_wait_2					= u32(1) << 0,
		possible_action_2_can_wait_1					= u32(1) << 1,
		possible_action_invalid							= u32(-1),
	};
	
public:
	typedef xr_vector<moving_object*>					NEAREST_MOVING;
	typedef std::pair<moving_object*,moving_object*>	COLLISION;
	typedef std::pair<possible_actions,COLLISION>		COLLISION_ACTION;
	typedef std::pair<float,COLLISION_ACTION>			COLLISION_TIME;
	typedef xr_vector<COLLISION_TIME>					COLLISIONS;

private:
	typedef xr_vector<ISpatial*>						Spatials;


public:
	typedef obstacles_query								query;
	typedef obstacles_query::AREA						AREA;

private:
	TREE						*m_tree;
	NEAREST_STATIC				m_nearest_static;

private:
	NEAREST_MOVING				m_nearest_moving;
	NEAREST_MOVING				m_collision_emitters;
	NEAREST_MOVING				m_visited_emitters;
	COLLISIONS					m_collisions;
	COLLISIONS					m_previous_collisions;
	Spatials					m_spatial_objects;

#ifdef DEBUG
private:
	typedef xr_set<moving_object*>						OBJECTS;

private:
	OBJECTS						m_objects;
#endif // DEBUG

private:
	IC		bool				collided				(const CObject *object, const Fvector &position, const float &radius) const;
			bool				collided_static			(const Fvector &position, const float &radius);
			bool				collided_static			(moving_object *object, const Fvector &dest_position);
			void				fill_static				(obstacles_query &query, const Fvector &position, const float &radius);
			void				fill_static				(obstacles_query &query);
			void				fill_all_static			(moving_object *object, const Fvector &dest_position);
			void				fill_nearest_list		(const Fvector &position, const float &radius, moving_object *object);

private:
	IC		MagicBox3			&continuous_box			(moving_object *object0, const Fvector &position0, MagicBox3 &result, const bool &use_box_enlargement) const;
			void				resolve_collision_previous	(boxes &current, moving_object *object0, moving_object *object1, possible_actions &action) const;
			void				resolve_collision_first	(boxes &current, moving_object *object0, moving_object *object1, possible_actions &action) const;
			void				resolve_collision		(boxes & current, moving_object *object0, const Fvector &position0, moving_object *object1, const Fvector &position1, possible_actions &action) const;
			bool				collided_dynamic		(moving_object *object0, const Fvector &position0, moving_object *object1, const Fvector &position1, boxes &result) const;
			bool				collided_dynamic		(moving_object *object0, const Fvector &position0, moving_object *object1, const Fvector &position1) const;
			bool 				collided_dynamic		(moving_object *object0, const Fvector &position0, moving_object *object1, const Fvector &position1, possible_actions &action) const;
			bool				exchange_all			(moving_object *previous, moving_object *next, const u32 &collision_count);
			bool 				fill_collisions			(moving_object *object, const Fvector &object_position, const float &time_to_check);
			void 				fill_nearest_moving		(moving_object *object);
			bool				already_wait			(moving_object *object) const;
			void 				generate_collisions		(moving_object *object);
			void				remove_already_waited	();
			void 				generate_emitters		();
			void 				resolve_collisions		();

public:
								moving_objects			();
								~moving_objects			();

public:
			void				register_object			(moving_object *moving_object);
			void				unregister_object		(moving_object *moving_object);

public:
			void				on_level_load			();
			void				on_object_move			(moving_object *moving_object);

public:
			void				query_action_dynamic	(moving_object *object);
			void				query_action_static		(moving_object *object, const Fvector &start_position, const Fvector &dest_position);
			void				query_action_static		(moving_object *object);
	IC		const COLLISIONS	&collisions				() const;
			void				clear					();
};

#include "moving_objects_inline.h"

#endif // MOVING_OBJECTS_H