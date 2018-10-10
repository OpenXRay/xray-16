#include "StdAfx.h"
#include "xrServer.h"

class xrClientData;
class CSE_Abstract;

xrClientData* xrServer::SelectBestClientToMigrateTo(CSE_Abstract* E, BOOL bForceAnother)
{
    return (xrClientData*)SV_Client;
    //
    //	if (bForceAnother)
    //	{
    //		// DUMB SELECTION - NOT THE CURRENT OWNER
    //		for (u32 it=0; it<net_Players.size(); ++it)
    //		{
    //			if (E->owner!=net_Players[it])	return (xrClientData*)net_Players[it];
    //		}
    //		return 0;
    //	} else {
    //		// DUMB SELECTION
    //		if (E->owner)	return E->owner;
    //		else			return (xrClientData*)net_Players[::Random.randI(0,net_Players.size())];
    //	}
}
