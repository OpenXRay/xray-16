////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_group_abstract.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife group abstract class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife.h"
#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_schedule_registry.h"
#include "alife_graph_registry.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/level_graph.h"

void CSE_ALifeGroupAbstract::switch_online()
{
    CSE_ALifeDynamicObject* object = smart_cast<CSE_ALifeDynamicObject*>(this);
    VERIFY(object);

    R_ASSERT(!object->m_bOnline);
    object->m_bOnline = true;

    ALife::OBJECT_IT I = m_tpMembers.begin(), B = I;
    ALife::OBJECT_IT E = m_tpMembers.end();
    u32 N = (u32)(E - I);
    for (; I != E; ++I)
    {
        CSE_ALifeDynamicObject* J = ai().alife().objects().object(*I);
        if (m_bCreateSpawnPositions)
        {
            J->o_Position = object->o_Position;
            J->m_tNodeID = object->m_tNodeID;
            CSE_ALifeMonsterAbstract* l_tpALifeMonsterAbstract = smart_cast<CSE_ALifeMonsterAbstract*>(J);
            if (l_tpALifeMonsterAbstract)
                l_tpALifeMonsterAbstract->o_torso.yaw = angle_normalize_signed((I - B) / N * PI_MUL_2);
        }
        object->alife().add_online(J, false);
    }
    m_bCreateSpawnPositions = false;
    object->alife().scheduled().remove(object);
    object->alife().graph().remove(object, object->m_tGraphID, false);
}

void CSE_ALifeGroupAbstract::switch_offline()
{
    CSE_ALifeDynamicObject* object = smart_cast<CSE_ALifeDynamicObject*>(base());
    VERIFY(object);

    R_ASSERT(object->m_bOnline);
    object->m_bOnline = false;

    ALife::OBJECT_IT I = m_tpMembers.begin();
    ALife::OBJECT_IT E = m_tpMembers.end();
    if (I != E)
    {
        CSE_ALifeMonsterAbstract* tpGroupMember =
            smart_cast<CSE_ALifeMonsterAbstract*>(ai().alife().objects().object(*I));
        CSE_ALifeMonsterAbstract* tpGroup = smart_cast<CSE_ALifeMonsterAbstract*>(this);
        if (tpGroupMember && tpGroup)
        {
            tpGroup->m_fCurSpeed = tpGroup->m_fCurrentLevelGoingSpeed;
            tpGroup->o_Position = tpGroupMember->o_Position;
            u32 dwNodeID = tpGroup->m_tNodeID;
            tpGroup->m_tGraphID = ai().cross_table().vertex(dwNodeID).game_vertex_id();
            tpGroup->m_fDistanceToPoint = ai().cross_table().vertex(dwNodeID).distance();
            tpGroup->m_tNextGraphID = tpGroup->m_tGraphID;
            u16 wNeighbourCount = ai().game_graph().vertex(tpGroup->m_tGraphID)->edge_count();
            CGameGraph::const_iterator i, e;
            ai().game_graph().begin(tpGroup->m_tGraphID, i, e);
            tpGroup->m_tPrevGraphID = (*(i + object->randI(0, wNeighbourCount))).vertex_id();
        }
        object->alife().remove_online(tpGroupMember, false);
        ++I;
    }
    for (; I != E; ++I)
        object->alife().remove_online(ai().alife().objects().object(*I), false);
    object->alife().scheduled().add(object);
    object->alife().graph().add(object, object->m_tGraphID, false);
}

bool CSE_ALifeGroupAbstract::synchronize_location()
{
    if (m_tpMembers.empty())
        return (true);

    CSE_ALifeDynamicObject* object = smart_cast<CSE_ALifeDynamicObject*>(base());
    VERIFY(object);

    ALife::OBJECT_VECTOR::iterator I = m_tpMembers.begin();
    ALife::OBJECT_VECTOR::iterator E = m_tpMembers.end();
    for (; I != E; ++I)
        ai().alife().objects().object(*I)->synchronize_location();

    CSE_ALifeDynamicObject& member = *ai().alife().objects().object(*I);
    object->o_Position = member.o_Position;
    object->m_tNodeID = member.m_tNodeID;

    if (object->m_tGraphID != member.m_tGraphID)
    {
        if (!object->m_bOnline)
            object->alife().graph().change(object, object->m_tGraphID, member.m_tGraphID);
        else
            object->m_tGraphID = member.m_tGraphID;
    }

    object->m_fDistance = member.m_fDistance;
    return (true);
}

void CSE_ALifeGroupAbstract::try_switch_online()
{
    CSE_ALifeDynamicObject* I = smart_cast<CSE_ALifeDynamicObject*>(base());
    VERIFY(I);

    // checking if the object is not an empty group of objects
    if (m_tpMembers.empty())
        return;

    I->try_switch_online();
}

void CSE_ALifeGroupAbstract::try_switch_offline()
{
    // checking if group is not empty
    if (m_tpMembers.empty())
        return;

    // so, we have a group of objects
    // therefore check all the group members if they are ready to switch offline

    CSE_ALifeDynamicObject* I = smart_cast<CSE_ALifeDynamicObject*>(base());
    VERIFY(I);

    // iterating on group members
    u32 memberCount = (u32)m_tpMembers.size();
    u32 i;
    for (i = 0; i < memberCount; ++i)
    {
        // casting group member to the abstract monster to get access to the Health property
        CSE_ALifeMonsterAbstract* tpGroupMember =
            smart_cast<CSE_ALifeMonsterAbstract*>(ai().alife().objects().object(m_tpMembers[i]));
        if (!tpGroupMember)
            continue;

        // check if monster is not dead
        if (tpGroupMember->g_Alive())
        {
            // so, monster is not dead
            // checking if the object is _not_ ready to switch offline
            if (!tpGroupMember->can_switch_offline())
                continue;

            if (!tpGroupMember->can_switch_online())
                // so, it is not ready, breaking a cycle, because we can't
                // switch group offline since not all the group members are ready
                // to switch offline
                break;

            if (I->alife().graph().actor()->o_Position.distance_to(tpGroupMember->o_Position) <=
                I->alife().offline_distance())
                // so, it is not ready, breaking a cycle, because we can't
                // switch group offline since not all the group members are ready
                // to switch offline
                break;

            continue;
        }

        // detach object from the group
        tpGroupMember->set_health(0.f);
        tpGroupMember->m_bDirectControl = true;
        m_tpMembers.erase(m_tpMembers.begin() + i);
        tpGroupMember->m_bOnline = false;
        CSE_ALifeInventoryItem* item = smart_cast<CSE_ALifeInventoryItem*>(tpGroupMember);
        if (item && item->attached())
        {
            CSE_ALifeDynamicObject* object = ai().alife().objects().object(tpGroupMember->ID_Parent, true);
            if (object)
                object->detach(item);
        }
        // store the __new separate object into the registries
        I->alife().register_object(tpGroupMember);

        // and remove it from the graph point but do not remove it from the current level map
        CSE_ALifeInventoryItem* l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(tpGroupMember);
        if (!l_tpALifeInventoryItem || !l_tpALifeInventoryItem->attached())
            I->alife().graph().remove(tpGroupMember, tpGroupMember->m_tGraphID, false);

        tpGroupMember->m_bOnline = true;
        --m_wCount;
        --i;
        --memberCount;
    }

    // checking if group is not empty
    if (m_tpMembers.empty())
        return;

    if (!I->can_switch_offline())
        return;

    if (I->can_switch_online() || (i == memberCount))
        I->alife().switch_offline(I);
}

bool CSE_ALifeGroupAbstract::redundant() const { return (m_tpMembers.empty()); }
