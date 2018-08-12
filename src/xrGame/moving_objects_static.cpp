////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects_static.cpp
//	Created 	: 27.03.2007
//  Modified 	: 14.05.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects with static objects, i.e stable dynamic objects
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "moving_objects.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "moving_object.h"
#include "moving_objects_impl.h"

bool moving_objects::collided_static(const Fvector& position, const float& radius)
{
    NEAREST_STATIC::const_iterator I = m_nearest_static.begin();
    NEAREST_STATIC::const_iterator E = m_nearest_static.end();
    for (; I != E; ++I)
    {
        if (collided(*I, position, radius))
            return (true);
    }

    return (false);
}

bool moving_objects::collided_static(moving_object* object, const Fvector& dest_position)
{
    float radius = object->radius() + ai().level_graph().header().cell_size() * .5f;
    float linear_velocity = dest_position.distance_to(object->position()) / time_to_check;
    float distance_to_check = time_to_check * linear_velocity;
    u32 step_count = iFloor(distance_to_check / step_to_check + .5f);
    for (u32 i = 0; i < step_count; ++i)
    {
        if (!i)
        {
            if (collided_static(object->position(), radius))
                return (true);

            continue;
        }

        if ((i + 1) == step_count)
        {
            if (collided_static(dest_position, radius))
                return (true);

            continue;
        }

        if (collided_static(object->predict_position(i * step_to_check), radius))
            return (true);
    }

    return (false);
}

void moving_objects::fill_static(obstacles_query& query)
{
    NEAREST_STATIC::const_iterator I = m_nearest_static.begin();
    NEAREST_STATIC::const_iterator E = m_nearest_static.end();
    for (; I != E; ++I)
        query.add(smart_cast<const CGameObject*>(*I));
}

void moving_objects::fill_static(obstacles_query& query, const Fvector& position, const float& radius)
{
    NEAREST_STATIC::const_iterator I = m_nearest_static.begin();
    NEAREST_STATIC::const_iterator E = m_nearest_static.end();
    for (; I != E; ++I)
    {
        if (!collided(*I, position, radius))
            continue;

        query.add(smart_cast<const CGameObject*>(*I));
    }
}

void moving_objects::fill_all_static(moving_object* object, const Fvector& dest_position)
{
    float radius = object->radius() + ai().level_graph().header().cell_size() * .5f;
    float linear_velocity = dest_position.distance_to(object->position()) / time_to_check;
    float distance_to_check = time_to_check * linear_velocity;
    u32 step_count = iFloor(distance_to_check / step_to_check + .5f);
    for (u32 i = 0; i < step_count; ++i)
    {
        if (!i)
        {
            fill_static(object->static_query(), object->position(), radius);
            continue;
        }

        if ((i + 1) == step_count)
        {
            fill_static(object->static_query(), dest_position, radius);
            continue;
        }

        fill_static(object->static_query(), object->predict_position(i * step_to_check), radius);
    }
}

class ignore_predicate
{
private:
    moving_object* m_object;

public:
    IC ignore_predicate(moving_object* object) : m_object(object) {}
    IC bool operator()(const IGameObject* object) const
    {
        if (m_object->ignored(object))
            return (true);

        const CGameObject* game_object = smart_cast<const CGameObject*>(object);
        VERIFY(game_object);
        if (!game_object->is_ai_obstacle())
            return (true);

        return (false);
    }
};

void moving_objects::fill_nearest_list(const Fvector& position, const float& radius, moving_object* object)
{
    Level().ObjectSpace.GetNearest(
        m_spatial_objects, m_nearest_static, position, radius, const_cast<CEntityAlive*>(&object->object()));

    m_nearest_static.erase(std::remove_if(m_nearest_static.begin(), m_nearest_static.end(), ignore_predicate(object)),
        m_nearest_static.end());
}

void moving_objects::query_action_static(
    moving_object* object, const Fvector& _start_position, const Fvector& dest_position)
{
    Fvector start_position = _start_position;
    start_position.average(dest_position);

    fill_nearest_list(start_position, dest_position.distance_to(start_position) + EPS, object);

    if (m_nearest_static.empty())
        return;

    if (!collided_static(object, dest_position))
        return;

    fill_nearest_list(start_position, dest_position.distance_to(start_position) + additional_radius + EPS, object);

    fill_static(object->static_query());
    //	fill_all_static			(object,dest_position);
}

void moving_objects::query_action_static(moving_object* object)
{
    query_action_static(object, object->position(), object->predict_position(time_to_check));
}
