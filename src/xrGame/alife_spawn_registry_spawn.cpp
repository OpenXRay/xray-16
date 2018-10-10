////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_spawn_registry_spawn.cpp
//	Created 	: 19.10.2004
//  Modified 	: 19.10.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife spawn registry spawn routines
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_spawn_registry.h"
#include "Random.hpp"

IC bool CALifeSpawnRegistry::enabled_spawn(CSE_Abstract& abstract) const
{
    return (!!abstract.m_spawn_flags.is(CSE_Abstract::flSpawnEnabled));
}

IC bool CALifeSpawnRegistry::count_limit(CSE_Abstract& abstract) const
{
    if (!!abstract.m_spawn_flags.is(CSE_Abstract::flSpawnInfiniteCount))
        return (false);

    //	if (abstract.m_spawn_count < abstract.m_max_spawn_count)
    //		return	(false);

    return (true);
}

IC bool CALifeSpawnRegistry::time_limit(CSE_Abstract& abstract, ALife::_TIME_ID game_time) const
{
    if (!!abstract.m_spawn_flags.is(CSE_Abstract::flSpawnOnSurgeOnly))
        return (false);

    //	if (game_time >= abstract.m_next_spawn_time)
    //		return	(false);

    return (true);
}

IC bool CALifeSpawnRegistry::spawned_item(CSE_Abstract& abstract, SPAWN_IDS& objects) const
{
    SPAWN_IDS::iterator I = std::lower_bound(objects.begin(), objects.end(), abstract.m_tSpawnID);
    return ((I != objects.end()) && (*I == abstract.m_tSpawnID));
}

IC bool CALifeSpawnRegistry::spawned_item(SPAWN_GRAPH::CVertex* vertex, SPAWN_IDS& objects)
{
    if (vertex->edges().empty())
        return (spawned_item(vertex->data()->object(), objects));

    SPAWN_GRAPH::const_iterator I = vertex->edges().begin();
    SPAWN_GRAPH::const_iterator E = vertex->edges().end();
    for (; I != E; ++I)
        if (spawned_item(m_spawns.vertex((*I).vertex_id()), objects))
            return (true);

    return (false);
}

IC bool CALifeSpawnRegistry::object_existance_limit(CSE_Abstract& abstract, SPAWN_IDS& objects) const
{
    if (!abstract.m_spawn_flags.is(CSE_Abstract::flSpawnIfDestroyedOnly))
        return (false);

    if (spawned_item(abstract, objects))
        return (true);

    return (false);
}

IC bool CALifeSpawnRegistry::can_spawn(CSE_Abstract& abstract, ALife::_TIME_ID game_time, SPAWN_IDS& objects) const
{
    return (enabled_spawn(abstract) && !count_limit(abstract) && !time_limit(abstract, game_time) &&
        !object_existance_limit(abstract, objects));
}

void CALifeSpawnRegistry::fill_new_spawns_single(
    SPAWN_GRAPH::CVertex* vertex, SPAWN_IDS& spawns, ALife::_TIME_ID game_time, SPAWN_IDS& objects)
{
    if (!!vertex->data()->object().m_spawn_flags.is(CSE_Abstract::flSpawnIfDestroyedOnly) &&
        spawned_item(vertex, objects))
        return;

    float accumulator = 0.f;
    SPAWN_GRAPH::const_iterator I = vertex->edges().begin(), B = I;
    SPAWN_GRAPH::const_iterator E = vertex->edges().end();
    for (; I != E; ++I)
        accumulator += (*I).weight();

    float probability = randF(accumulator);
    //	float						group_probability = vertex->data()->object().m_spawn_probability;
    float group_probability = 1.f;

    if (probability >= accumulator * group_probability)
        return;

    accumulator = 0.f;
    I = B;
    for (; I != E; ++I)
    {
        accumulator += (*I).weight() * group_probability;
        if (accumulator > probability)
        {
            //			vertex->data()->object().m_spawn_count++;
            fill_new_spawns(m_spawns.vertex((*I).vertex_id()), spawns, game_time, objects);
            return;
        }
    }
}

void CALifeSpawnRegistry::fill_new_spawns(
    SPAWN_GRAPH::CVertex* vertex, SPAWN_IDS& spawns, ALife::_TIME_ID game_time, SPAWN_IDS& objects)
{
    VERIFY(vertex);
    if (!can_spawn(vertex->data()->object(), game_time, objects))
        return;

    if (vertex->edges().empty())
    {
        //		vertex->data()->object().m_spawn_count++;
        spawns.push_back(vertex->data()->object().m_tSpawnID);
        return;
    }

    if (!!vertex->data()->object().m_spawn_flags.is(CSE_Abstract::flSpawnSingleItemOnly))
    {
        fill_new_spawns_single(vertex, spawns, game_time, objects);
        return;
    }

    //	vertex->data()->object().m_spawn_count++;

    SPAWN_GRAPH::const_iterator I = vertex->edges().begin();
    SPAWN_GRAPH::const_iterator E = vertex->edges().end();
    for (; I != E; ++I)
        if (randF(1.f) < (*I).weight())
            fill_new_spawns(m_spawns.vertex((*I).vertex_id()), spawns, game_time, objects);
}

void CALifeSpawnRegistry::fill_new_spawns(SPAWN_IDS& spawns, ALife::_TIME_ID game_time, SPAWN_IDS& objects)
{
    process_spawns(objects);

    SPAWN_IDS::iterator I = m_spawn_roots.begin();
    SPAWN_IDS::iterator E = m_spawn_roots.end();
    for (; I != E; ++I)
        fill_new_spawns(m_spawns.vertex(*I), spawns, game_time, objects);

    process_spawns(spawns);
}
