#include "stdafx.h"
#include "xrServer.h"

// Initiate migration
void xrServer::PerformMigration(CSE_Abstract* E, xrClientData* from, xrClientData* to)
{
	return;
//	R_ASSERT	(from != to);
//	NET_Packet	P;
//
//	// Send to current 'client' signal to deactivate 'entity'
//	{
//		P.w_begin			(M_MIGRATE_DEACTIVATE);
//		P.w_u16				(E->ID);
//		SendTo				(from->ID,P,net_flags(TRUE,TRUE));
//	}
//
//	// Send to _new_ 'client' signal to activate 'entity'
//	{
//		P.w_begin			(M_MIGRATE_ACTIVATE);
//		P.w_u16				(E->ID);
//		E->UPDATE_Write		(P);
//		SendTo				(to->ID,P,net_flags(TRUE,TRUE));
//	}
//
//	// Change parent-client
//	E->owner				= to;
}
