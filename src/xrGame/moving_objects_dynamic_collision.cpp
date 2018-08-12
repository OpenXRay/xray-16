////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects_dynamic_collision.cpp
//	Created 	: 27.03.2007
//  Modified 	: 13.06.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects with dynamic objects collision, i.e. objects with predictable behaviour
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "moving_objects.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "moving_object.h"
#include "moving_objects_impl.h"
#include "magic_box3.h"
#include "ai_obstacle.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

#if 0
#define MSG(...) Msg(__VA_ARGS__)
#else
#define MSG(...)
#endif

IC MagicBox3& moving_objects::continuous_box(
    moving_object* object, const Fvector& position, MagicBox3& result, const bool& use_box_enlargement) const
{
    result = object->object().obstacle().min_box();
    object->object().Center(result.Center());
    result.Center().add(Fvector().sub(position, object->position()));

    if (use_box_enlargement && (object->action() != moving_object::action_move))
    {
        result.Extent(0) *= wait_radius_factor;
        result.Extent(2) *= wait_radius_factor;
    }

    return (result);
}

struct collision_predicate
{
    const moving_objects::COLLISION* m_collision;

    IC collision_predicate(const moving_objects::COLLISION& collision) : m_collision(&collision) {}
    IC bool operator()(const moving_objects::COLLISION_TIME& collision_time) const
    {
        const moving_objects::COLLISION& object = collision_time.second.second;
        if (m_collision->first != object.first)
            return (false);

        if (m_collision->second != object.second)
            return (false);

        return (true);
    }
};

struct boxes
{
    MagicBox3 _0;
    MagicBox3 _1;
};

void moving_objects::resolve_collision_first(
    boxes& current, moving_object* object0, moving_object* object1, possible_actions& action) const
{
    boxes start;
    continuous_box(object0, object0->position(), start._0, false);
    continuous_box(object1, object1->position(), start._1, false);

    boxes target;
    continuous_box(object0, object0->target_position(), target._0, false);
    continuous_box(object1, object1->target_position(), target._1, false);

    if (target._0.intersects(start._1))
    {
        MSG("%6d [t0s1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_1_can_wait_2;
        return;
    }

    if (target._1.intersects(start._0))
    {
        MSG("%6d [t1s0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_2_can_wait_1;
        return;
    }

    if (start._0.intersects(current._1))
    {
        MSG("%6d [s0c1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_2_can_wait_1;
        return;
    }

    if (start._1.intersects(current._0))
    {
        MSG("%6d [s1c0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_1_can_wait_2;
        return;
    }

    if (current._0.intersects(target._1))
    {
        MSG("%6d [c0t1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_2_can_wait_1;
        return;
    }

    if (current._1.intersects(target._0))
    {
        MSG("%6d [c1t0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        action = possible_action_1_can_wait_2;
        return;
    }

    action = possible_action_1_can_wait_2;
}

void moving_objects::resolve_collision_previous(
    boxes& current, moving_object* object0, moving_object* object1, possible_actions& action) const
{
    boxes start;
    continuous_box(object0, object0->position(), start._0, true);
    continuous_box(object1, object1->position(), start._1, true);

    boxes target;
    continuous_box(object0, object0->target_position(), target._0, true);
    continuous_box(object1, object1->target_position(), target._1, true);

    action = possible_action_1_can_wait_2;
    if (target._0.intersects(start._1))
    {
        MSG("%6d [t0s1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        return;
    }

    if (start._1.intersects(current._0))
    {
        MSG("%6d [s1c0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        return;
    }

    if (current._1.intersects(target._0))
    {
        MSG("%6d [c1t0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
        return;
    }

    action = possible_action_2_can_wait_1;
    if (action == possible_action_2_can_wait_1)
    {
        if (target._1.intersects(start._0))
        {
            MSG("%6d [t1s0] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
            return;
        }

        if (start._0.intersects(current._1))
        {
            MSG("%6d [s0c1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
            return;
        }

        if (current._0.intersects(target._1))
        {
            MSG("%6d [c0t1] [%s][%s]", Device.dwFrame, *object0->object().cName(), *object1->object().cName());
            return;
        }
    }

    action = possible_action_1_can_wait_2;
}

void moving_objects::resolve_collision(boxes& current, moving_object* object0, const Fvector& position0,
    moving_object* object1, const Fvector& position1, possible_actions& action) const
{
#if 0
	if (object0->action_frame() == Device.dwFrame) {
		Msg						("%6d Oooooooops",Device.dwFrame);
		{
			Msg					("  visited emitters[%d]",m_visited_emitters.size());
			NEAREST_MOVING::const_iterator	I = m_visited_emitters.begin();
			NEAREST_MOVING::const_iterator	E = m_visited_emitters.end();
			for ( ; I != E; ++I) {
				Msg				("    %s",(*I)->object().cName().c_str());
			}
		}
		{
			Msg					("  collision emitters[%d]",m_collision_emitters.size());
			NEAREST_MOVING::const_iterator	I = m_collision_emitters.begin();
			NEAREST_MOVING::const_iterator	E = m_collision_emitters.end();
			for ( ; I != E; ++I) {
				Msg				("    %s",(*I)->object().cName().c_str());
			}
		}
		{
			Msg					("  nearest moving[%d]",m_nearest_moving.size());
			NEAREST_MOVING::const_iterator	I = m_nearest_moving.begin();
			NEAREST_MOVING::const_iterator	E = m_nearest_moving.end();
			for ( ; I != E; ++I) {
				Msg				("    %s",(*I)->object().cName().c_str());
			}
		}
		Msg						("%6d Eng of \"Oooooooops\"",Device.dwFrame);
	}
#endif // 0
    VERIFY2(
        object0->action_frame() != Device.dwFrame, make_string("%d %s", Device.dwFrame, *object0->object().cName()));
    VERIFY2(object0->action_frame() < Device.dwFrame, make_string("%d %s", Device.dwFrame, *object0->object().cName()));

    VERIFY2(
        object1->action_frame() != Device.dwFrame, make_string("%d %s", Device.dwFrame, *object0->object().cName()));
    VERIFY2(object1->action_frame() < Device.dwFrame, make_string("%d %s", Device.dwFrame, *object0->object().cName()));

    bool first_time = (std::find_if(m_previous_collisions.begin(), m_previous_collisions.end(),
                           collision_predicate(std::make_pair(object0, object1))) == m_previous_collisions.end());

    if (first_time)
    {
        resolve_collision_first(current, object0, object1, action);
        return;
    }

    if (object0->action() == moving_object::action_wait)
    {
        resolve_collision_previous(current, object0, object1, action);
        return;
    }

    if (object1->action() == moving_object::action_wait)
    {
        resolve_collision_previous(current, object1, object0, action);
        action = possible_actions(action ^ (possible_action_1_can_wait_2 | possible_action_2_can_wait_1));
        return;
    }

    VERIFY2(false, make_string("NODEFAULT: [%s][%s]", *object0->object().cName(), *object1->object().cName()));
}

bool moving_objects::collided_dynamic(moving_object* object0, const Fvector& position0, moving_object* object1,
    const Fvector& position1, boxes& result) const
{
    continuous_box(object0, position0, result._0, true);
    continuous_box(object1, position1, result._1, true);
    return (result._0.intersects(result._1));
}

bool moving_objects::collided_dynamic(
    moving_object* object0, const Fvector& position0, moving_object* object1, const Fvector& position1) const
{
    boxes temp;
    return (collided_dynamic(object0, position0, object1, position1, temp));
}

bool moving_objects::collided_dynamic(moving_object* object0, const Fvector& position0, moving_object* object1,
    const Fvector& position1, possible_actions& action) const
{
#ifdef DEBUG
    COLLISIONS::const_iterator I = m_collisions.begin();
    COLLISIONS::const_iterator E = m_collisions.end();
    for (; I != E; ++I)
    {
        VERIFY(!already_wait(object0));
        VERIFY(!already_wait(object1));
    }
#endif // DEBUG

    boxes current;
    if (!collided_dynamic(object0, position0, object1, position1, current))
        return (false);

    resolve_collision(current, object0, position0, object1, position1, action);
    return (true);
}
