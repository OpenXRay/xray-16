#include "stdafx.h"
#include "xrserver.h"
#include "xrserver_objects.h"

void xrServer::Process_event_activate	(NET_Packet& P, const ClientID sender, const u32 time, const u16 id_parent, const u16 id_entity, bool send_message)
{
	// Parse message
	CSE_Abstract*		e_parent	= game->get_entity_from_eid	(id_parent);
	CSE_Abstract*		e_entity	= game->get_entity_from_eid	(id_entity);

#ifndef MASTER_GOLD
	Msg("---Artefact activate (parent = %d) (item = %d)", id_parent, id_entity);
#endif // #ifndef MASTER_GOLD
	
	R_ASSERT2			(e_parent, make_string("parent not found. id_parent=%d id_entity=%d frame=%d",id_parent,id_entity, Device.dwFrame).c_str());
	R_ASSERT2			(e_entity, make_string("entity not found. id_parent=%d id_entity=%d frame=%d",id_parent,id_entity, Device.dwFrame).c_str());

	if (!game->OnActivate(id_parent, id_entity))
		return;


	if (0xffff == e_entity->ID_Parent) 
	{
#ifndef MASTER_GOLD
		Msg	("~ ERROR: can't activate independant object. entity[%s:%d], parent[%s:%d], section[%s]",
			e_entity->name_replace(),id_entity,e_parent->name_replace(),id_parent, *e_entity->s_name);
#endif // #ifndef MASTER_GOLD
		return;
	}

	// Signal to everyone (including sender)
	if (send_message)
	{
		DWORD MODE		= net_flags(TRUE,TRUE, FALSE, TRUE);
		SendBroadcast	(BroadcastCID,P,MODE);
	}
	
	return;
}