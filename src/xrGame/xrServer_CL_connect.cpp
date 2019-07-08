#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrserver_objects.h"
#include "xrServer_Objects_Alife_Monsters.h"
#include "Level.h"

void xrServer::Perform_connect_spawn(CSE_Abstract* E, xrClientData* CL, NET_Packet& P)
{
	if (E->net_Processed)
		return;
	if (E->s_flags.is(M_SPAWN_OBJECT_PHANTOM))	
		return;
	
	E->net_Processed = TRUE;

	P.B.count = 0;

    // Connectivity order
    CSE_Abstract* Parent = ID_to_entity(E->ID_Parent);
    if (Parent)
        Perform_connect_spawn(Parent, CL, P);

	// Process
	Flags16 save = E->s_flags;
	E->s_flags.set(M_SPAWN_UPDATE,TRUE);
	
	if (!E->owner)	
	{
		if (E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
		{
			CL->owner = E;
			E->set_name_replace("player");
		}

        // Associate
        E->owner = CL;
        E->Spawn_Write(P, TRUE);
        E->UPDATE_Write(P);

        CSE_ALifeObject* object = smart_cast<CSE_ALifeObject*>(E);
        VERIFY(object);
        if (!object->keep_saved_data_anyway())
            object->client_data.clear();
    }
    else
    {
        E->Spawn_Write(P, FALSE);
        E->UPDATE_Write(P);
    }

    E->s_flags = save;
    SendTo(CL->ID, P);
}

void xrServer::SendConfigFinished(ClientID const& clientId)
{
    NET_Packet P;
    P.w_begin(M_SV_CONFIG_FINISHED);
    SendTo(clientId, P);
}

void xrServer::SendConnectionData(IClient* _CL)
{
	xrClientData* CL = (xrClientData*)_CL;
	NET_Packet P;

	// Replicate current entities on to this client
	xrS_entities::iterator I = entities.begin(), E = entities.end();
	for (; I!=E; ++I)		
		Perform_connect_spawn(I->second, CL, P);

	SendConfigFinished(CL->ID);
};

void xrServer::OnCL_Connected(IClient* _CL)
{
	xrClientData* CL = (xrClientData*)_CL;
	CL->net_Accepted = TRUE;

	Export_game_type(CL);
	Perform_game_export();
	SendConnectionData(CL);

	// game->OnPlayerConnect(CL->ID);	
}
