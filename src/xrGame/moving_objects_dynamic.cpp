
//	Module 		: moving_objects_dynamic.cpp
//	Created 	: 27.03.2007
//  Modified 	: 14.05.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects with dynamic objects, i.e. objects with predictable behaviour
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "moving_objects.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "moving_object.h"
#include "moving_objects_impl.h"
#include "magic_box3.h"
#include "ai_obstacle.h"

#ifndef MASTER_GOLD
#include "ai_debug.h"
#endif // MASTER_GOLD

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

extern MagicBox3 MagicMinBox(int iQuantity, const Fvector* akPoint);

struct priority
{
    IC static bool standing(const moving_object* object)
    {
        if (object->action() == moving_object::action_wait)
            return (false);

        return (!!fis_zero(object->position().distance_to_sqr(object->predict_position(1.f))));
    }

    IC static bool predicate(const moving_object* _0, const moving_object* _1)
    {
        if (standing(_0))
        {
            if (standing(_1))
                return (_0 < _1);

            return (false);
        }

        if (standing(_1))
            return (true);

        return (_0 < _1);
    }

    IC static bool predicate2(const moving_objects::COLLISION_TIME& _0, const moving_objects::COLLISION_TIME& _1)
    {
        if (_0.first < _1.first)
            return (true);

        if (_1.first < _0.first)
            return (false);

        if (predicate(_0.second.second.first, _1.second.second.first))
            return (true);

        if (predicate(_1.second.second.first, _0.second.second.first))
            return (false);

        return (predicate(_0.second.second.second, _1.second.second.second));
    }
};

struct collision
{
    const moving_object* m_object;

    IC collision(const moving_object* object) : m_object(object) {}
    IC bool operator()(const moving_objects::COLLISION_TIME& object) const
    {
        if (object.second.second.first == m_object)
            return (true);

        if (object.second.second.second == m_object)
            return (true);

        return (false);
    }
};

struct already_wait_predicate
{
    const moving_objects::COLLISIONS* m_collisions;

    IC already_wait_predicate(const moving_objects::COLLISIONS& collisions) : m_collisions(&collisions) {}
    IC bool operator()(moving_object* object) const
    {
        moving_objects::COLLISIONS::const_iterator I =
            std::find_if(m_collisions->begin(), m_collisions->end(), collision(object));
        if (I == m_collisions->end())
            return (false);

        if ((*I).second.second.first == object)
        {
            if ((*I).second.first == moving_objects::possible_action_1_can_wait_2)
                return (true);

            return (false);
        }

        if ((*I).second.second.second == object)
        {
            if ((*I).second.first == moving_objects::possible_action_2_can_wait_1)
                return (true);

            return (false);
        }

        return (false);
    }
};

void moving_objects::fill_nearest_moving(moving_object* object)
{
    Fvector next_position = object->predict_position(time_to_check);
    float linear_velocity = next_position.distance_to(object->position()) / time_to_check;
    float radius = (max_linear_velocity + linear_velocity) * time_to_check;
    m_tree->nearest(object->position(), radius, m_nearest_moving);
#if 0
	Msg							("%6d nearest moving[%d] object[%s]", Device.dwFrame, m_nearest_moving.size(),object->object().cName().c_str());
	{
		NEAREST_MOVING::const_iterator	I = m_nearest_moving.begin();
		NEAREST_MOVING::const_iterator	E = m_nearest_moving.end();
		for ( ; I != E; ++I) {
			Msg					("    %s",(*I)->object().cName().c_str());
		}
	}
#endif // 0

    if (m_nearest_moving.empty())
        return;

    struct already_computed
    {
        static IC bool predicate(moving_object* const& object) { return (object->action_frame() == Device.dwFrame); }
    };

    m_nearest_moving.erase(
        std::remove_if(m_nearest_moving.begin(), m_nearest_moving.end(), &already_computed::predicate),
        m_nearest_moving.end());

    u32 size = m_nearest_moving.size();
    moving_object** temp = (moving_object**)_alloca(size * sizeof(moving_object*));
    std::copy(m_nearest_moving.begin(), m_nearest_moving.end(), temp);
    std::sort(temp, temp + size);
    m_nearest_moving.erase(std::set_difference(temp, temp + size, m_visited_emitters.begin(), m_visited_emitters.end(),
                               m_nearest_moving.begin()),
        m_nearest_moving.end());
}

void moving_objects::remove_already_waited()
{
    m_nearest_moving.erase(
        std::remove_if(m_nearest_moving.begin(), m_nearest_moving.end(), already_wait_predicate(m_collisions)),
        m_nearest_moving.end());
}

void moving_objects::generate_emitters()
{
    remove_already_waited();

    std::sort(m_collision_emitters.begin(), m_collision_emitters.end());

// it should be alredy sorted here
//	std::sort					(m_nearest_moving.begin(),m_nearest_moving.end());
#ifdef DEBUG
    if (!m_nearest_moving.empty())
    {
        NEAREST_MOVING::const_iterator I = m_nearest_moving.begin(), J = I + 1;
        NEAREST_MOVING::const_iterator E = m_nearest_moving.end();
        for (; J != E; ++I, ++J)
        {
            VERIFY(*I < *J);
        }
    }
#endif // DEBUG

    u32 size = m_collision_emitters.size();
    m_collision_emitters.resize(size + m_nearest_moving.size());

    m_collision_emitters.erase(
        std::set_difference(m_nearest_moving.begin(), m_nearest_moving.end(), m_collision_emitters.begin(),
            m_collision_emitters.begin() + size, m_collision_emitters.begin() + size),
        m_collision_emitters.end());

    std::inplace_merge(m_collision_emitters.begin(), m_collision_emitters.begin() + size, m_collision_emitters.end());
}

bool moving_objects::exchange_all(moving_object* previous, moving_object* next, const u32& collision_count)
{
    bool result = true;

    VERIFY(m_collisions.size() >= collision_count);
    COLLISIONS::iterator I = m_collisions.begin();
    COLLISIONS::iterator E = I + collision_count;
    for (; I != E; ++I)
    {
        if ((*I).second.second.first == previous)
        {
            if ((*I).second.first != possible_action_2_can_wait_1)
            {
                result = false;
                continue;
            }

            (*I).second.second.first = next;
            continue;
        }

        if ((*I).second.second.second != previous)
            continue;

        if ((*I).second.first != possible_action_1_can_wait_2)
        {
            result = false;
            continue;
        }

        (*I).second.second.second = next;
        continue;
    }

    return (result);
}

bool moving_objects::fill_collisions(moving_object* object, const Fvector& object_position, const float& time_to_check)
{
    possible_actions action;
    int i = 0;
    NEAREST_MOVING::const_iterator I = m_nearest_moving.begin();
    NEAREST_MOVING::const_iterator E = m_nearest_moving.end();
    for (; I != E; ++I)
    {
        ++i;
        bool break_cycle = false;

        bool priority = ::priority::predicate(object, *I);
        if (priority)
        {
            if (!collided_dynamic(object, object_position, (*I), (*I)->predict_position(time_to_check), action))
                continue;

            if (action == possible_action_1_can_wait_2)
                break_cycle = true;
            else
                VERIFY(action == possible_action_2_can_wait_1);
        }
        else
        {
            if (!collided_dynamic((*I), (*I)->predict_position(time_to_check), object, object_position, action))
                continue;

            if (action == possible_action_2_can_wait_1)
                break_cycle = true;
            else
                VERIFY(action == possible_action_1_can_wait_2);
        }

        m_collisions.push_back(std::make_pair(time_to_check,
            std::make_pair(action, std::make_pair(priority ? object : (*I), !priority ? object : (*I)))));

        if (break_cycle)
            return (false);
    }

    {
        u32 collision_count = m_collisions.size();
        COLLISION_TIME* collisions = (COLLISION_TIME*)_alloca(collision_count * sizeof(COLLISION_TIME));
        std::copy(m_collisions.begin(), m_collisions.end(), collisions);
        COLLISION_TIME* I = collisions;
        COLLISION_TIME* E = collisions + collision_count;
        for (; I != E; ++I)
        {
            moving_object* test;
            if ((*I).second.first == possible_action_1_can_wait_2)
                test = (*I).second.second.first;
            else
            {
                VERIFY((*I).second.first == possible_action_2_can_wait_1);
                test = (*I).second.second.second;
            }

            bool priority = ::priority::predicate(object, test);
            if (priority)
            {
                if (!collided_dynamic(object, object_position, test, test->position()))
                    continue;
            }
            else
            {
                if (!collided_dynamic(test, test->position(), object, object_position))
                    continue;
            }

            m_collisions.push_back(std::make_pair(
                time_to_check, std::make_pair(priority ? possible_action_2_can_wait_1 : possible_action_1_can_wait_2,
                                   std::make_pair(priority ? object : test, !priority ? object : test))));
        }

        VERIFY(m_collisions.size() >= collision_count);
        COLLISIONS::iterator b = m_collisions.begin() + collision_count, i = b;
        COLLISIONS::iterator e = m_collisions.end();
        for (; i != e; ++i)
        {
            if ((*i).second.second.first == object)
            {
                if (exchange_all((*i).second.second.second, object, collision_count))
                    continue;

                (*i).second.second.first = 0;
                continue;
            }

            VERIFY((*i).second.second.second == object);
            if (!exchange_all((*i).second.second.first, object, collision_count))
                (*i).second.second.first = 0;
        }

        struct remove
        {
            static IC bool predicate(const COLLISION_TIME& collision) { return (!collision.second.second.first); }
        };

        m_collisions.erase(std::remove_if(b, e, &remove::predicate), e);
    }

    return (true);
}

bool moving_objects::already_wait(moving_object* object) const
{
    moving_objects::COLLISIONS::const_iterator I =
        std::find_if(m_collisions.begin(), m_collisions.end(), collision(object));
    if (I == m_collisions.end())
        return (false);

    if ((*I).second.second.first == object)
    {
        if ((*I).second.first == moving_objects::possible_action_1_can_wait_2)
            return (true);

        return (false);
    }

    if ((*I).second.second.second == object)
    {
        if ((*I).second.first == moving_objects::possible_action_2_can_wait_1)
            return (true);

        return (false);
    }

    return (false);
}

void moving_objects::generate_collisions(moving_object* object)
{
    if (m_nearest_moving.empty())
        return;

    if (already_wait(object))
        return;

    Fvector dest_position = object->predict_position(time_to_check);
    float linear_velocity = dest_position.distance_to(object->position()) / time_to_check;
    float distance_to_check = time_to_check * linear_velocity;
    u32 step_count = iFloor(distance_to_check / step_to_check + .5f);
    for (u32 i = 0; (i < step_count) && !m_nearest_moving.empty(); ++i, remove_already_waited())
    {
        if (!i)
        {
            if (!fill_collisions(object, object->position(), 0.f))
                return;

            continue;
        }

        if ((i + 1) == step_count)
        {
            if (!fill_collisions(object, dest_position, i * step_to_check))
                return;

            continue;
        }

        if (!fill_collisions(object, object->predict_position(i * step_to_check), i * step_to_check))
            return;
    }
}

typedef std::pair<moving_object*, moving_object::action_type> decision;

struct decision_predicate
{
    const moving_object* m_object;

    IC decision_predicate(const moving_object* object) : m_object(object) {}
    IC bool operator()(const decision& decision) const { return (m_object == decision.first); }
};

void moving_objects::resolve_collisions()
{
    std::sort(m_collisions.begin(), m_collisions.end(), &priority::predicate2);

    m_previous_collisions = m_collisions;

    u32 collidee_count = m_collisions.size() * 2 + m_visited_emitters.size();
    moving_object** collidees = (moving_object**)_alloca(collidee_count * sizeof(moving_object*));
    {
        moving_object** J = collidees;
        {
            COLLISIONS::const_iterator I = m_collisions.begin();
            COLLISIONS::const_iterator E = m_collisions.end();
            for (; I != E; ++I)
            {
                *J = (*I).second.second.first;
                ++J;
                *J = (*I).second.second.second;
                ++J;
            }
        }
        {
            NEAREST_MOVING::const_iterator I = m_visited_emitters.begin();
            NEAREST_MOVING::const_iterator E = m_visited_emitters.end();
            for (; I != E; ++I, ++J)
                *J = *I;
        }

        std::sort(collidees, collidees + collidee_count);
        collidee_count = u32(std::unique(collidees, collidees + collidee_count) - collidees);
    }

    u32 decision_count = collidee_count;
    decision* decisions = (decision*)_alloca(decision_count * sizeof(decision));

    {
        moving_object** I = collidees;
        moving_object** E = collidees + collidee_count;
        decision* J = decisions;
        for (; I != E; ++I, ++J)
            *J = std::make_pair(*I, moving_object::action_move);
    }

    {
        COLLISIONS::const_iterator I = m_collisions.begin();
        COLLISIONS::const_iterator E = m_collisions.end();
        for (; I != E; ++I)
        {
            switch ((*I).second.first)
            {
            case possible_action_1_can_wait_2:
            {
                decision* object =
                    std::find_if(decisions, decisions + decision_count, decision_predicate((*I).second.second.first));
                VERIFY(object != (decisions + decision_count));
                *object = std::make_pair(object->first, moving_object::action_wait);
                //					(*I).second.second.second->dynamic_query().merge((*I).second.second.first->dynamic_query());
                (*I).second.second.second->dynamic_query().add(&object->first->object());
                continue;
            }
            case possible_action_2_can_wait_1:
            {
                decision* object =
                    std::find_if(decisions, decisions + decision_count, decision_predicate((*I).second.second.second));
                VERIFY(object != (decisions + decision_count));
                *object = std::make_pair(object->first, moving_object::action_wait);
                //					(*I).second.second.first->dynamic_query().merge((*I).second.second.second->dynamic_query());
                (*I).second.second.first->dynamic_query().add(&object->first->object());
                continue;
            }
            default: NODEFAULT;
            }
        }
    }

    {
        decision* I = decisions;
        decision* E = decisions + decision_count;
        for (; I != E; ++I)
            (*I).first->action((*I).second);
    }
}

void moving_objects::query_action_dynamic(moving_object* object)
{
#ifndef MASTER_GOLD
    if (psAI_Flags.test(aiObstaclesAvoidingStatic))
        return;

    if (object->action_frame() == Device.dwFrame)
        return;

    m_visited_emitters.clear();
    m_collision_emitters.clear();
    m_collisions.clear();

    m_collision_emitters.push_back(object);
    while (!m_collision_emitters.empty())
    {
        moving_object* object = m_collision_emitters.back();
        m_collision_emitters.pop_back();

        m_visited_emitters.insert(
            std::lower_bound(m_visited_emitters.begin(), m_visited_emitters.end(), object), object);

        fill_nearest_moving(object);
        generate_emitters();
        generate_collisions(object);
    }

    {
        NEAREST_MOVING::iterator I = m_visited_emitters.begin();
        NEAREST_MOVING::iterator E = m_visited_emitters.end();
        for (; I != E; ++I)
            (*I)->dynamic_query().clear();
    }

#if 0 // def DEBUG
	Msg							("%6d end of iteration", Device.dwFrame);
#endif // DEBUG

    if (!m_collisions.empty())
    {
        resolve_collisions();
        return;
    }

    m_previous_collisions = m_collisions;

#if 0 // def DEBUG
	{
		Msg							("Frame[%d], collisions[%d]",Device.dwFrame, m_visited_emitters.size());
		NEAREST_MOVING::iterator	I = m_visited_emitters.begin();
		NEAREST_MOVING::iterator	E = m_visited_emitters.end();
		for ( ; I != E; ++I)
			Msg						("  %s",*(*I)->object().cName());
	}
#endif // DEBUG

    NEAREST_MOVING::iterator I = m_visited_emitters.begin();
    NEAREST_MOVING::iterator E = m_visited_emitters.end();
    for (; I != E; ++I)
        (*I)->action(moving_object::action_move);

#endif // MASTER_GOLD
}
