////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_graph_registry.cpp
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife graph registry
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_graph_registry.h"
#include "xrServerEntities/xrMessages.h"

using namespace ALife;

CALifeGraphRegistry::CALifeGraphRegistry()
{
    m_level = 0;
    m_process_time = 0;
    m_actor = 0;
}

CALifeGraphRegistry::~CALifeGraphRegistry() { xr_delete(m_level); }
void CALifeGraphRegistry::on_load()
{
    for (int i = 0; i < GameGraph::LOCATION_TYPE_COUNT; ++i)
    {
        {
            for (int j = 0; j < GameGraph::LOCATION_COUNT; ++j)
                m_terrain[i][j].clear();
        }
        for (GameGraph::_GRAPH_ID j = 0; j < (GameGraph::_GRAPH_ID)ai().game_graph().header().vertex_count(); ++j)
            m_terrain[i][ai().game_graph().vertex(j)->vertex_type()[i]].push_back(j);
    }

    m_objects.resize(ai().game_graph().header().vertex_count());

    {
        GRAPH_REGISTRY::iterator I = m_objects.begin();
        GRAPH_REGISTRY::iterator E = m_objects.end();
        for (; I != E; ++I)
            (*I).objects().clear();
    }
}

void CALifeGraphRegistry::update(CSE_ALifeDynamicObject* object)
{
    if (!object->m_bDirectControl)
        return;

    if (object->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
    {
        m_actor = smart_cast<CSE_ALifeCreatureActor*>(object);
        R_ASSERT2(m_actor, "Invalid flag M_SPAWN_OBJECT_ASPLAYER for non-actor object!");

        if (g_start_game_vertex_id)
        {
            m_actor->m_tGraphID = g_start_game_vertex_id;
            m_actor->o_Position = g_start_position;
        }
    }

    if (m_actor && !m_level)
        setup_current_level();

    CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(object);
    if (!item || !item->attached())
        add(object, object->m_tGraphID);
}

void CALifeGraphRegistry::setup_current_level()
{
    m_level = xr_new<CALifeLevelRegistry>(ai().game_graph().vertex(actor()->m_tGraphID)->level_id());
    level().set_process_time(m_process_time);
    for (int i = 0, n = ai().game_graph().header().vertex_count(); i < n; ++i)
        if (ai().game_graph().vertex(i)->level_id() == level().level_id())
        {
            D_OBJECT_P_MAP::const_iterator I = m_objects[i].objects().objects().begin();
            D_OBJECT_P_MAP::const_iterator E = m_objects[i].objects().objects().end();
            for (; I != E; ++I)
                level().add((*I).second);
        }

    {
        xr_vector<CSE_ALifeDynamicObject*>::const_iterator I = m_temp.begin();
        xr_vector<CSE_ALifeDynamicObject*>::const_iterator E = m_temp.end();
        for (; I != E; ++I)
            level().add(*I);

        m_temp.clear();
    }
    GameGraph::LEVEL_MAP::const_iterator I =
        ai().game_graph().header().levels().find(ai().game_graph().vertex(actor()->m_tGraphID)->level_id());
    R_ASSERT2(ai().game_graph().header().levels().end() != I, "Graph point level ID not found!");

    [[maybe_unused]] const int id = g_pGamePersistent->Level_ID(I->second.name().c_str(), "1.0", true);
    VERIFY3(id >= 0, "Level is corrupted or doesn't exist", I->second.name().c_str());
    ai().load(I->second.name().c_str());

    g_start_game_vertex_id = 0;
}

void CALifeGraphRegistry::attach(CSE_Abstract& object, CSE_ALifeInventoryItem* item,
    GameGraph::_GRAPH_ID game_vertex_id, bool alife_query, bool add_children)
{
#ifdef DEBUG
    if (psAI_Flags.test(aiALife))
    {
        Msg("[LSS] Attaching item [%s][%d] to [%s][%d]", item->base()->name_replace(), item->base()->ID,
            object.name_replace(), object.ID);
    }
#endif
    if (alife_query)
        remove(smart_cast<CSE_ALifeDynamicObject*>(item), game_vertex_id);
    else
        level().remove(smart_cast<CSE_ALifeDynamicObject*>(item));

    CSE_ALifeDynamicObject* dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(&object);
    R_ASSERT2(!alife_query || dynamic_object, "Cannot attach an item to a non-alife object object");

    dynamic_object->attach(item, alife_query, add_children);
}

void CALifeGraphRegistry::detach(CSE_Abstract& object, CSE_ALifeInventoryItem* item,
    GameGraph::_GRAPH_ID game_vertex_id, bool alife_query, bool remove_children)
{
#ifdef DEBUG
    if (psAI_Flags.test(aiALife))
    {
        Msg("[LSS] Detaching item [%s][%d] from [%s][%d]", item->base()->name_replace(), item->base()->ID,
            object.name_replace(), object.ID);
    }
#endif
    if (alife_query)
        add(smart_cast<CSE_ALifeDynamicObject*>(item), game_vertex_id);
    else
    {
        CSE_ALifeDynamicObject* object = smart_cast<CSE_ALifeDynamicObject*>(item);
        VERIFY(object);
        object->m_tGraphID = game_vertex_id;
        level().add(object);
    }

    CSE_ALifeDynamicObject* dynamic_object = smart_cast<CSE_ALifeDynamicObject*>(&object);
    R_ASSERT2(!alife_query || dynamic_object, "Cannot detach an item from non-alife object");

    VERIFY(alife_query || !smart_cast<CSE_ALifeDynamicObject*>(&object) ||
        (ai().game_graph().vertex(smart_cast<CSE_ALifeDynamicObject*>(&object)->m_tGraphID)->level_id() ==
            level().level_id()));

    if (dynamic_object)
        dynamic_object->detach(item, 0, alife_query, remove_children);
    else
    {
#ifdef DEBUG
        bool value =
            std::find(object.children.begin(), object.children.end(), item->base()->ID) != object.children.end();
        if (!value)
        {
            Msg("! ERROR: can't detach independant object. entity[%s:%d], parent[%s:%d], section[%s]",
                item->base()->name_replace(), item->base()->ID, object.name_replace(), object.ID,
                item->base()->s_name.c_str());
        }
#endif // DEBUG
        //		R_ASSERT2				(value,"Can't detach an item which is not on my own");
    }
}

void CALifeGraphRegistry::add(CSE_ALifeDynamicObject* object, GameGraph::_GRAPH_ID game_vertex_id, bool update)
{
#ifdef DEBUG
    if (psAI_Flags.test(aiALife))
    {
        Msg("[LSS] adding object [%s][%d] to graph point %d", object->name_replace(), object->ID, game_vertex_id);
    }
#endif
    if (!object->m_bOnline && object->used_ai_locations() /**&& object->interactive()**/)
    {
        VERIFY(ai().game_graph().valid_vertex_id(game_vertex_id));
        OBJECT_REGISTRY& target = m_objects[game_vertex_id].objects();
        const auto& target_map = target.objects();
        const auto existing_at_target = target_map.find(object->ID);
        const bool already_registered_here =
            (existing_at_target != target_map.end() && existing_at_target->second == object);

        if (!already_registered_here)
        {
            const GameGraph::_GRAPH_ID vertex_count = (GameGraph::_GRAPH_ID)m_objects.size();
            for (GameGraph::_GRAPH_ID i = 0; i < vertex_count; ++i)
            {
                if (!ai().game_graph().valid_vertex_id(i))
                    continue;
                OBJECT_REGISTRY& reg = m_objects[i].objects();
                const auto& om = reg.objects();
                if (om.find(object->ID) == om.end())
                    continue;
                reg.remove(object->ID, true);
            }
            target.add(object->ID, object);
        }
        object->m_tGraphID = game_vertex_id;
    }
    else if (!m_level && update)
    {
        m_temp.push_back(object);
        object->m_tGraphID = game_vertex_id;
    }

    if (update && m_level && ai().game_graph().valid_vertex_id(game_vertex_id))
        level().add(object);
}

void CALifeGraphRegistry::remove(CSE_ALifeDynamicObject* object, GameGraph::_GRAPH_ID game_vertex_id, bool update)
{
    bool removed_from_graph = false;
    if (object->used_ai_locations() /**&& object->interactive()**/)
    {
#ifdef DEBUG
        if (psAI_Flags.test(aiALife))
        {
            Msg("[LSS] removing object [%s][%d] from graph point %d", object->name_replace(), object->ID,
                game_vertex_id);
        }
#endif
        if (ai().game_graph().valid_vertex_id(game_vertex_id))
        {
            OBJECT_REGISTRY& primary = m_objects[game_vertex_id].objects();
            const auto& primary_map = primary.objects();
            if (primary_map.find(object->ID) != primary_map.end())
            {
                primary.remove(object->ID);
                removed_from_graph = true;
            }
        }
        if (!removed_from_graph)
        {
            const GameGraph::_GRAPH_ID vertex_count = (GameGraph::_GRAPH_ID)m_objects.size();
            for (GameGraph::_GRAPH_ID i = 0; i < vertex_count; ++i)
            {
                if (!ai().game_graph().valid_vertex_id(i))
                    continue;
                if (i == game_vertex_id)
                    continue;
                OBJECT_REGISTRY& reg = m_objects[i].objects();
                const auto& om = reg.objects();
                if (om.find(object->ID) == om.end())
                    continue;
                reg.remove(object->ID);
                removed_from_graph = true;
#ifndef MASTER_GOLD
                Msg("! [ALife] graph registry: object [%s][%d] was at vertex %u, not at m_tGraphID %u — removed from "
                    "actual vertex",
                    object->name_replace(), object->ID, i, game_vertex_id);
#endif
                break;
            }
        }
#ifndef MASTER_GOLD
        if (!removed_from_graph)
            Msg("! [ALife] graph registry: remove [%s][%d] — not in any vertex map (expected %u), continuing",
                object->name_replace(), object->ID, game_vertex_id);
#endif
    }
    if (update && m_level)
    {
        bool level_no_assert =
            ai().game_graph().vertex(game_vertex_id)->level_id() != level().level_id();
        if (object->used_ai_locations() && !removed_from_graph)
            level_no_assert = true;
        level().remove(object, level_no_assert);
    }
}
