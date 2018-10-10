#include "StdAfx.h"
#include "xrServer.h"
#include "xrServer_Objects.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "xrServer_svclient_validation.h"
#include "xrNetServer/NET_Messages.h"
#include "xrServerEntities/xrMessages.h"

void ReplaceOwnershipHeader(NET_Packet& P)
{
    //способ очень грубый, но на данный момент иного выбора нет. Заранее приношу извинения
    u16 NewType = GE_OWNERSHIP_TAKE;
    CopyMemory(&P.B.data[6], &NewType, 2);
};

void xrServer::Process_event_ownership(NET_Packet& P, ClientID sender, u32 time, u16 ID, BOOL bForced)
{
    u32 MODE = net_flags(TRUE, TRUE, FALSE, TRUE);

    u16 id_parent = ID, id_entity;
    P.r_u16(id_entity);
    CSE_Abstract* e_parent = game->get_entity_from_eid(id_parent);
    CSE_Abstract* e_entity = game->get_entity_from_eid(id_entity);

#ifdef MP_LOGGING
    Msg("--- SV: Process ownership take: parent [%d][%s], item [%d][%s]", id_parent,
        e_parent ? e_parent->name_replace() : "null_parent", id_entity, e_entity ? e_entity->name() : "null_entity");
#endif // MP_LOGGING

    if (!e_parent)
    {
        Msg("! ERROR on ownership: parent not found. parent_id = [%d], entity_id = [%d], frame = [%d].", id_parent,
            id_entity, Device.dwFrame);
        return;
    }
    if (!e_entity)
    {
        //		Msg( "! ERROR on ownership: entity not found. parent_id = [%d], entity_id = [%d], frame = [%d].",
        // id_parent, id_entity, Device.dwFrame );
        return;
    }

    if (!is_object_valid_on_svclient(id_parent))
    {
        Msg("! ERROR on ownership: parent object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame "
            "= "
            "[%d]",
            id_parent, id_entity, Device.dwFrame);
        return;
    }

    if (!is_object_valid_on_svclient(id_entity))
    {
        Msg("! ERROR on ownership: entity object is not valid on sv client. parent_id = [%d], entity_id = [%d], frame "
            "= "
            "[%d]",
            id_parent, id_entity, Device.dwFrame);
        return;
    }

    if (0xffff != e_entity->ID_Parent)
        return;

    xrClientData* c_parent = e_parent->owner;
    xrClientData* c_entity = e_entity->owner;
    xrClientData* c_from = ID_to_client(sender);

    if ((GetServerClient() != c_from) && (c_parent != c_from))
    {
        // trust only ServerClient or new_ownerClient
        return;
    }

    CSE_ALifeCreatureAbstract* alife_entity = smart_cast<CSE_ALifeCreatureAbstract*>(e_parent);
    if (alife_entity && !alife_entity->g_Alive() && game->Type() != eGameIDSingle)
    {
#ifdef MP_LOGGING
        Msg("--- SV: WARNING: dead player [%d] tries to take item [%d]", id_parent, id_entity);
#endif //#ifdef MP_LOGGING
        return;
    };

    // Game allows ownership of entity
    if (game->OnTouch(id_parent, id_entity, bForced))
    {
        // Perform migration if needed
        if (c_parent != c_entity)
            PerformMigration(e_entity, c_entity, c_parent);

        // Rebuild parentness
        e_entity->ID_Parent = id_parent;
        e_parent->children.push_back(id_entity);

        if (bForced)
        {
            ReplaceOwnershipHeader(P);
        }
        // Signal to everyone (including sender)
        SendBroadcast(BroadcastCID, P, MODE);
    }
}
