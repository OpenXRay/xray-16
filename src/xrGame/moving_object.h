////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects.h
//	Created 	: 28.03.2007
//  Modified 	: 28.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects
////////////////////////////////////////////////////////////////////////////

#ifndef MOVING_OBJECT_H
#define MOVING_OBJECT_H

#include "entity_alive.h"
#include "obstacles_query.h"

class moving_object {
public:
	enum action_type {
		action_move			= u32(0),
		action_wait,
		action_step_left,
		action_step_right,
		action_step_backward,
		action_step_forward,
		action_follow,
	};

private:
	const CEntityAlive			*m_object;
	const CObject				*m_ignored_object;
	Fvector						m_position;
	obstacles_query				m_static_query;
	obstacles_query				m_dynamic_query;

private:
	action_type					m_action;
	Fvector						m_action_position;
	u32							m_action_frame;
	u32							m_action_time;

public:
								moving_object	(const CEntityAlive *object);
								~moving_object	();
			void				on_object_move	();
	IC		shared_str			id				() const;
	IC		const Fvector		&position		() const;
	IC		float				radius			() const;
			Fvector				predict_position(const float &time_to_check) const;
			Fvector				target_position	() const;
			void				update_position	();
	IC		const CEntityAlive	&object			() const;
			void				ignore			(const CObject *object);
			bool				ignored			(const CObject *object);
	IC		void				action			(const action_type &action);
	IC		void				action			(const action_type &action, const Fvector &action_position);
	IC		const action_type	&action			() const;
	IC		const Fvector		&action_position() const;
	IC		const u32			&action_frame	() const;
	IC		const u32			&action_time	() const;
	IC		obstacles_query		&static_query	();
	IC		obstacles_query		&dynamic_query	();
};

#include "moving_object_inline.h"

#endif // MOVING_OBJECT_H