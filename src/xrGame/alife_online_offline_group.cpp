////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_online_offline_group.cpp
//	Created 	: 25.10.2005
//  Modified 	: 25.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife Online Offline Group class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "alife_graph_registry.h"
#include "alife_schedule_registry.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "alife_online_offline_group_brain.h"
#include "xrAICore/Navigation/level_graph.h"
#include "alife_monster_movement_manager.h"
#include "alife_monster_detail_path_manager.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

extern void setup_location_types_line(GameGraph::TERRAIN_VECTOR& m_vertex_types, LPCSTR string);

CSE_ALifeItemWeapon* CSE_ALifeOnlineOfflineGroup::tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower)
{
    return (0);
}

ALife::EMeetActionType CSE_ALifeOnlineOfflineGroup::tfGetActionType(
    CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex, bool bMutualDetection)
{
    return (ALife::eMeetActionTypeIgnore);
}

bool CSE_ALifeOnlineOfflineGroup::bfActive() { return (!m_bOnline && !m_members.empty()); }
CSE_ALifeDynamicObject* CSE_ALifeOnlineOfflineGroup::tpfGetBestDetector() { return (0); }
bool CSE_ALifeOnlineOfflineGroup::need_update(CSE_ALifeDynamicObject* object) { return true; }
void CSE_ALifeOnlineOfflineGroup::update()
{
    if (m_bOnline)
    {
        MEMBER* commander = (*m_members.begin()).second;
        o_Position = commander->o_Position;
        m_tNodeID = commander->m_tNodeID;
        m_tGraphID = commander->m_tGraphID;
    }
    if (!bfActive())
        return;

    brain().update();

    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        MEMBER* m = (*I).second;
        m->o_Position = o_Position;
        m->m_tNodeID = m_tNodeID;
        m->m_tGraphID = m_tGraphID;
        m->m_fDistance = m_fDistance;
    }
    return;
}

void CSE_ALifeOnlineOfflineGroup::on_location_change() const { brain().on_location_change(); }
void CSE_ALifeOnlineOfflineGroup::register_member(ALife::_OBJECT_ID member_id)
{
    VERIFY(m_members.find(member_id) == m_members.end());
    CSE_ALifeDynamicObject* object = ai().alife().objects().object(member_id);
    CSE_ALifeMonsterAbstract* monster = smart_cast<CSE_ALifeMonsterAbstract*>(object);
    VERIFY(monster);
    VERIFY(monster->g_Alive());

    bool empty = m_members.empty();
    if (!object->m_bOnline)
    {
        if (m_bOnline)
        {
            object->switch_online();
            VERIFY(object->ID_Parent == 0xffff);
            alife().graph().level().remove(object);
        }
        else
        {
            alife().graph().remove(object, object->m_tGraphID);
            alife().scheduled().remove(object);
        }
    }
    else
    {
        if (!m_bOnline)
        {
            switch_online();
        }
        VERIFY(object->ID_Parent == 0xffff);
        alife().graph().level().remove(object);
    }
    VERIFY((monster->m_group_id == 0xffff) || (monster->m_group_id == ID));
    monster->m_group_id = ID;
    m_members.insert(std::make_pair(member_id, monster));

    if (!empty)
        return;

    o_Position = monster->o_Position;
    m_tNodeID = monster->m_tNodeID;
    m_tGraphID = monster->m_tGraphID;
    m_fGoingSpeed = monster->m_fGoingSpeed;
    m_fCurrentLevelGoingSpeed = monster->m_fCurrentLevelGoingSpeed;
    m_flags.set(flUsedAI_Locations, TRUE);
    alife().graph().update(this);
}

void CSE_ALifeOnlineOfflineGroup::unregister_member(ALife::_OBJECT_ID member_id)
{
    CALifeGraphRegistry& graph = alife().graph();
    //	CALifeLevelRegistry			&level = graph.level();

    MEMBERS::iterator I = m_members.find(member_id);
    VERIFY(I != m_members.end());
    VERIFY((*I).second->m_group_id == ID);
    (*I).second->m_group_id = 0xffff;

    graph.update((*I).second);
    alife().scheduled().add((*I).second);
    m_members.erase(I);

    if (m_members.empty())
    {
        m_flags.set(flUsedAI_Locations, FALSE);
    }
}

CSE_ALifeOnlineOfflineGroup::MEMBER* CSE_ALifeOnlineOfflineGroup::member(ALife::_OBJECT_ID member_id, bool no_assert)
{
    MEMBERS::iterator I = m_members.find(member_id);
    if (I == m_members.end())
    {
        if (!no_assert)
            Msg("! There is no member with id %d in the OnlineOfflineGroup id %d", member_id, ID);
        VERIFY(no_assert);
        return (0);
    }
    return ((*I).second);
}

bool CSE_ALifeOnlineOfflineGroup::synchronize_location()
{
    if (m_bOnline)
    {
        MEMBER* member = (*m_members.begin()).second;
        o_Position = member->o_Position;
        m_tNodeID = member->m_tNodeID;
        m_tGraphID = member->m_tGraphID;
        m_fDistance = member->m_fDistance;
    }

    return (true);
}

void CSE_ALifeOnlineOfflineGroup::try_switch_online()
{
    if (m_members.empty())
        return;

    if (!can_switch_online())
        return;

    if (!can_switch_offline())
    {
        inherited1::try_switch_online();
        return;
    }
    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        VERIFY3((*I).second->g_Alive(), "Incorrect situation : some of the OnlineOffline group members is dead",
            (*I).second->name_replace());
        VERIFY3((*I).second->can_switch_online(),
            "Incorrect situation : some of the OnlineOffline group members cannot be switched online due to their "
            "personal "
            "properties",
            (*I).second->name_replace());
        VERIFY3((*I).second->can_switch_offline(),
            "Incorrect situation : some of the OnlineOffline group members cannot be switched online due to their "
            "personal "
            "properties",
            (*I).second->name_replace());
        if (alife().graph().actor()->o_Position.distance_to((*I).second->o_Position) > alife().online_distance())
        {
            continue;
        }
        inherited1::try_switch_online();
        return;
    }
    on_failed_switch_online();
}

void CSE_ALifeOnlineOfflineGroup::try_switch_offline()
{
    if (m_members.empty())
        return;

    if (!can_switch_offline())
        return;

    if (!can_switch_online())
    {
        alife().switch_offline(this);
        return;
    }

    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        VERIFY3((*I).second->g_Alive(), "Incorrect situation : some of the OnlineOffline group members is dead",
            (*I).second->name_replace());
        VERIFY3((*I).second->can_switch_offline(),
            "Incorrect situation : some of the OnlineOffline group members cannot be switched online due to their "
            "personal "
            "properties",
            (*I).second->name_replace());
        VERIFY3((*I).second->can_switch_online(),
            "Incorrect situation : some of the OnlineOffline group members cannot be switched online due to their "
            "personal "
            "properties",
            (*I).second->name_replace());

        if (alife().graph().actor()->o_Position.distance_to((*I).second->o_Position) <= alife().offline_distance())
            return;
    }

    alife().switch_offline(this);
}

void CSE_ALifeOnlineOfflineGroup::switch_online()
{
    R_ASSERT(!m_bOnline);
    m_bOnline = true;

    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        if ((*I).second->m_bOnline == false)
            alife().add_online((*I).second, false);
    }

    alife().scheduled().remove(this);
    alife().graph().remove(this, m_tGraphID, false);
}

void CSE_ALifeOnlineOfflineGroup::switch_offline()
{
    R_ASSERT(m_bOnline);
    m_bOnline = false;

    if (!m_members.empty())
    {
        MEMBER* member = (*m_members.begin()).second;

        member->synchronize_location();

        o_Position = member->o_Position;
        m_tNodeID = member->m_tNodeID;
        m_tGraphID = member->m_tGraphID;
        m_fDistance = member->m_fDistance;
    }

    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        if ((*I).second->m_bOnline == true)
        {
            (*I).second->clear_client_data();
            alife().remove_online((*I).second, false);
        }
    }

    alife().scheduled().add(this);
    alife().graph().add(this, m_tGraphID, false);
}

bool CSE_ALifeOnlineOfflineGroup::redundant() const { return (m_members.empty()); }
void CSE_ALifeOnlineOfflineGroup::notify_on_member_death(MEMBER* member) { unregister_member(member->ID); }
void CSE_ALifeOnlineOfflineGroup::on_before_register()
{
    m_tGraphID = GameGraph::_GRAPH_ID(-1);
    m_flags.set(flUsedAI_Locations, FALSE);
}

void CSE_ALifeOnlineOfflineGroup::on_after_game_load()
{
    if (m_members.empty())
        return;

    ALife::_OBJECT_ID* temp = (ALife::_OBJECT_ID*)_alloca(m_members.size() * sizeof(ALife::_OBJECT_ID));
    ALife::_OBJECT_ID *i = temp, *e = temp + m_members.size();

    {
        MEMBERS::const_iterator I = m_members.begin();
        MEMBERS::const_iterator E = m_members.end();
        for (; I != E; ++I, ++i)
        {
            VERIFY(!(*I).second);
            *i = (*I).first;
        }
    }

    m_members.clear();

    for (i = temp; i != e; ++i)
        register_member(*i);
}

ALife::_OBJECT_ID CSE_ALifeOnlineOfflineGroup::commander_id()
{
    if (!m_members.empty())
        return (*m_members.begin()).first;
    return 0xffff;
}

CSE_ALifeOnlineOfflineGroup::MEMBERS const& CSE_ALifeOnlineOfflineGroup::squad_members() const { return m_members; }
u32 CSE_ALifeOnlineOfflineGroup::npc_count() const { return m_members.size(); }
void CSE_ALifeOnlineOfflineGroup::clear_location_types()
{
    m_tpaTerrain.clear();
    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        (*I).second->m_tpaTerrain.clear();
    }
}

void CSE_ALifeOnlineOfflineGroup::add_location_type(LPCSTR mask)
{
    setup_location_types_line(m_tpaTerrain, mask);
    MEMBERS::iterator I = m_members.begin();
    MEMBERS::iterator E = m_members.end();
    for (; I != E; ++I)
    {
        setup_location_types_line((*I).second->m_tpaTerrain, mask);
    }
}

void CSE_ALifeOnlineOfflineGroup::force_change_position(Fvector position)
{
    u32 new_level_vertex = ai().level_graph().vertex_id(position);
    GameGraph::_GRAPH_ID new_graph_vertex = ai().cross_table().vertex(new_level_vertex).game_vertex_id();
    o_Position = position;
    m_tNodeID = new_level_vertex;
    if (m_tGraphID != new_graph_vertex)
    {
        alife().graph().change(this, m_tGraphID, new_graph_vertex);
    }

    // m_tGraphID				= new_graph_vertex;
}

void CSE_ALifeOnlineOfflineGroup::on_failed_switch_online()
{
    MEMBERS::const_iterator I = m_members.begin();
    MEMBERS::const_iterator E = m_members.end();
    for (; I != E; ++I)
    {
        (*I).second->clear_client_data();
    }
}
