#include "StdAfx.h"
#include "xrServer.h"
#include "xrServer_Objects.h"
#include "xrNetServer/NET_Messages.h"

bool xrServer::Process_event_reject(
    NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message)
{
    // Parse message
    CSE_Abstract* e_parent = game->get_entity_from_eid(id_parent);
    CSE_Abstract* e_entity = game->get_entity_from_eid(id_entity);

    //	R_ASSERT2( e_entity, make_string( "entity not found. parent_id = [%d], entity_id = [%d], frame = [%d]",
    // id_parent, id_entity, Device.dwFrame ).c_str() );
    VERIFY2(e_entity, make_string("entity not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent,
                          id_entity, Device.dwFrame)
                          .c_str());
    if (!e_entity)
    {
        Msg("! ERROR on rejecting: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.dwFrame);
        return false;
    }

    //	R_ASSERT2( e_parent, make_string( "parent not found. parent_id = [%d], entity_id = [%d], frame = [%d]",
    // id_parent, id_entity, Device.dwFrame ).c_str() );
    VERIFY2(e_parent, make_string("parent not found. parent_id = [%d], entity_id = [%d], frame = [%d]", id_parent,
                          id_entity, Device.dwFrame)
                          .c_str());
    if (!e_parent)
    {
        Msg("! ERROR on rejecting: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.dwFrame);
        return false;
    }

#ifdef MP_LOGGING
    Msg("--- SV: Process reject: parent[%d][%s], item[%d][%s]", id_parent, e_parent->name_replace(), id_entity,
        e_entity->name());
#endif // MP_LOGGING

    xr_vector<u16>& C = e_parent->children;
    xr_vector<u16>::iterator c = std::find(C.begin(), C.end(), id_entity);
    if (c == C.end())
    {
        Msg("! WARNING: SV: can't find children [%d] of parent [%d]", id_entity, e_parent);
        return false;
    }

    if (0xffff == e_entity->ID_Parent)
    {
#ifndef MASTER_GOLD
        Msg("! ERROR: can't detach independant object. entity[%s][%d], parent[%s][%d], section[%s]",
            e_entity->name_replace(), id_entity, e_parent->name_replace(), id_parent, e_entity->s_name.c_str());
#endif // #ifndef MASTER_GOLD
        return (false);
    }

    // Rebuild parentness
    //.	Msg("---ID_Parent [%d], id_parent [%d]", e_entity->ID_Parent, id_parent);

    // R_ASSERT(e_entity->ID_Parent == id_parent);
    if (e_entity->ID_Parent != id_parent)
    {
        Msg("! ERROR: e_entity->ID_Parent = [%d]  parent = [%d][%s]  entity_id = [%d]  frame = [%d]",
            e_entity->ID_Parent, id_parent, e_parent->name_replace(), id_entity, Device.dwFrame);
        // it can't be !!!
    }

    game->OnDetach(id_parent, id_entity);

    // R_ASSERT3(C.end()!=c,e_entity->name_replace(),e_parent->name_replace());
    e_entity->ID_Parent = 0xffff;
    C.erase(c);

    // Signal to everyone (including sender)
    if (send_message)
    {
        u32 MODE = net_flags(TRUE, TRUE, FALSE, TRUE);
        SendBroadcast(BroadcastCID, P, MODE);
    }

    return (true);
}
