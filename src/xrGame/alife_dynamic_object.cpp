////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_dynamic_object.cpp
//	Created 	: 27.10.2005
//  Modified 	: 27.10.2005
//	Author		: Dmitriy Iassenev
//	Description : ALife dynamic object class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xrServer_Objects_ALife.h"
#include "alife_simulator.h"
#include "alife_schedule_registry.h"
#include "alife_graph_registry.h"
#include "alife_object_registry.h"
#include "xrAICore/Navigation/level_graph.h"
#include "xrAICore/Navigation/game_level_cross_table.h"
#include "xrAICore/Navigation/game_graph.h"
#include "xrServer.h"

void CSE_ALifeDynamicObject::on_spawn()
{
#ifdef DEBUG
//	Msg			("[LSS] spawning object [%d][%d][%s][%s]",ID,ID_Parent,name(),name_replace());
#endif
}

void CSE_ALifeDynamicObject::on_register()
{
    CSE_ALifeObject* object = this;
    while (object->ID_Parent != ALife::_OBJECT_ID(-1))
    {
        object = ai().alife().objects().object(object->ID_Parent);
        VERIFY(object);
    }

    if (!alife().graph().level().object(object->ID, true))
        clear_client_data();
}

void CSE_ALifeDynamicObject::on_before_register() {}
#include "Level.h"
#include "map_manager.h"

void CSE_ALifeDynamicObject::on_unregister() { Level().MapManager().OnObjectDestroyNotify(ID); }
void CSE_ALifeDynamicObject::switch_online()
{
    R_ASSERT(!m_bOnline);
    m_bOnline = true;
    alife().add_online(this);
}

void CSE_ALifeDynamicObject::switch_offline()
{
    R_ASSERT(m_bOnline);
    m_bOnline = false;
    alife().remove_online(this);

    clear_client_data();
}

void CSE_ALifeDynamicObject::add_online(const bool& update_registries)
{
    if (!update_registries)
        return;

    alife().scheduled().remove(this);
    alife().graph().remove(this, m_tGraphID, false);
}

void CSE_ALifeDynamicObject::add_offline(
    const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
{
    if (!update_registries)
        return;

    alife().scheduled().add(this);
    alife().graph().add(this, m_tGraphID, false);
}

bool CSE_ALifeDynamicObject::synchronize_location()
{
    if (!ai().level_graph().valid_vertex_position(o_Position) ||
        ai().level_graph().inside(ai().level_graph().vertex(m_tNodeID), o_Position))
        return (true);

    u32 const new_vertex_id = ai().level_graph().vertex(m_tNodeID, o_Position);
    if (!m_bOnline && !ai().level_graph().inside(new_vertex_id, o_Position))
        return (true);

    m_tNodeID = new_vertex_id;
    GameGraph::_GRAPH_ID tGraphID = ai().cross_table().vertex(m_tNodeID).game_vertex_id();
    if (tGraphID != m_tGraphID)
    {
        if (!m_bOnline)
        {
            Fvector position = o_Position;
            u32 level_vertex_id = m_tNodeID;
            alife().graph().change(this, m_tGraphID, tGraphID);
            if (ai().level_graph().inside(ai().level_graph().vertex(level_vertex_id), position))
            {
                level_vertex_id = m_tNodeID;
                o_Position = position;
            }
        }
        else
        {
            VERIFY(ai().game_graph().vertex(tGraphID)->level_id() == alife().graph().level().level_id());
            m_tGraphID = tGraphID;
        }
    }

    m_fDistance = ai().cross_table().vertex(m_tNodeID).distance();

    return (true);
}

void CSE_ALifeDynamicObject::try_switch_online()
{
    CSE_ALifeSchedulable* schedulable = smart_cast<CSE_ALifeSchedulable*>(this);
    // checking if the abstract monster has just died
    if (schedulable)
    {
        if (!schedulable->need_update(this))
        {
            if (alife().scheduled().object(ID, true))
                alife().scheduled().remove(this);
        }
        else if (!alife().scheduled().object(ID, true))
            alife().scheduled().add(this);
    }

    if (!can_switch_online())
    {
        on_failed_switch_online();
        return;
    }

    if (!can_switch_offline())
    {
        alife().switch_online(this);
        return;
    }

    if (alife().graph().actor()->o_Position.distance_to(o_Position) > alife().online_distance())
    {
        on_failed_switch_online();
        return;
    }

    alife().switch_online(this);
}

void CSE_ALifeDynamicObject::try_switch_offline()
{
    if (!can_switch_offline())
        return;

    if (!can_switch_online())
    {
        alife().switch_offline(this);
        return;
    }

    if (alife().graph().actor()->o_Position.distance_to(o_Position) <= alife().offline_distance())
        return;

    alife().switch_offline(this);
}

bool CSE_ALifeDynamicObject::redundant() const { return (false); }
/// ---------------------------- CSE_ALifeInventoryBox ---------------------------------------------

void CSE_ALifeInventoryBox::add_online(const bool& update_registries)
{
    CSE_ALifeDynamicObjectVisual* object = (this);

    NET_Packet tNetPacket;
    ClientID clientID;
    clientID.set(
        object->alife().server().GetServerClient() ? object->alife().server().GetServerClient()->ID.value() : 0);

    ALife::OBJECT_IT I = object->children.begin();
    ALife::OBJECT_IT E = object->children.end();
    for (; I != E; ++I)
    {
        CSE_ALifeDynamicObject* l_tpALifeDynamicObject = ai().alife().objects().object(*I);
        CSE_ALifeInventoryItem* l_tpALifeInventoryItem = smart_cast<CSE_ALifeInventoryItem*>(l_tpALifeDynamicObject);
        R_ASSERT2(l_tpALifeInventoryItem, "Non inventory item object has parent?!");
        l_tpALifeInventoryItem->base()->s_flags._or (M_SPAWN_UPDATE);
        CSE_Abstract* l_tpAbstract = smart_cast<CSE_Abstract*>(l_tpALifeInventoryItem);
        object->alife().server().entity_Destroy(l_tpAbstract);

#ifdef DEBUG
        //		if (psAI_Flags.test(aiALife))
        //			Msg					("[LSS] Spawning item
        //[%s][%s][%d]",l_tpALifeInventoryItem->base()->name_replace(),*l_tpALifeInventoryItem->base()->s_name,l_tpALifeDynamicObject->ID);
        Msg("[LSS][%d] Going online [%d][%s][%d] with parent [%d][%s] on '%s'", Device.dwFrame, Device.dwTimeGlobal,
            l_tpALifeInventoryItem->base()->name_replace(), l_tpALifeInventoryItem->base()->ID, ID, name_replace(),
            "*SERVER*");
#endif

        l_tpALifeDynamicObject->o_Position = object->o_Position;
        l_tpALifeDynamicObject->m_tNodeID = object->m_tNodeID;
        object->alife().server().Process_spawn(tNetPacket, clientID, FALSE, l_tpALifeInventoryItem->base());
        l_tpALifeDynamicObject->s_flags._and (u16(-1) ^ M_SPAWN_UPDATE);
        l_tpALifeDynamicObject->m_bOnline = true;
    }

    CSE_ALifeDynamicObjectVisual::add_online(update_registries);
}

void CSE_ALifeInventoryBox::add_offline(
    const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries)
{
    CSE_ALifeDynamicObjectVisual* object = (this);

    for (u32 i = 0, n = saved_children.size(); i < n; ++i)
    {
        CSE_ALifeDynamicObject* child =
            smart_cast<CSE_ALifeDynamicObject*>(ai().alife().objects().object(saved_children[i], true));
        R_ASSERT(child);
        child->m_bOnline = false;

        CSE_ALifeInventoryItem* inventory_item = smart_cast<CSE_ALifeInventoryItem*>(child);
        VERIFY2(inventory_item, "Non inventory item object has parent?!");
#ifdef DEBUG
        //		if (psAI_Flags.test(aiALife))
        //			Msg					("[LSS] Destroying item
        //[%s][%s][%d]",inventory_item->base()->name_replace(),*inventory_item->base()->s_name,inventory_item->base()->ID);
        Msg("[LSS][%d] Going offline [%d][%s][%d] with parent [%d][%s] on '%s'", Device.dwFrame, Device.dwTimeGlobal,
            inventory_item->base()->name_replace(), inventory_item->base()->ID, ID, name_replace(), "*SERVER*");
#endif

        ALife::_OBJECT_ID item_id = inventory_item->base()->ID;
        inventory_item->base()->ID = object->alife().server().PerformIDgen(item_id);

        if (!child->can_save())
        {
            object->alife().release(child);
            --i;
            --n;
            continue;
        }
        child->clear_client_data();
        object->alife().graph().add(child, child->m_tGraphID, false);
        //		object->alife().graph().attach	(*object,inventory_item,child->m_tGraphID,true);
        alife().graph().remove(child, child->m_tGraphID);
        children.push_back(child->ID);
        child->ID_Parent = ID;
    }

    CSE_ALifeDynamicObjectVisual::add_offline(saved_children, update_registries);
}

void CSE_ALifeDynamicObject::clear_client_data()
{
#ifdef DEBUG
    if (!client_data.empty())
        Msg("CSE_ALifeDynamicObject::switch_offline: client_data is cleared for [%d][%s]", ID, name_replace());
#endif // DEBUG
    if (!keep_saved_data_anyway())
        client_data.clear();
}

void CSE_ALifeDynamicObject::on_failed_switch_online() { clear_client_data(); }
