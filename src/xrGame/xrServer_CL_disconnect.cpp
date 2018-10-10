#include "StdAfx.h"
#include "xrServer.h"
#include "game_sv_single.h"
#include "alife_simulator.h"
#include "xrServer_Objects.h"
#include "Level.h"

void xrServer::OnCL_Disconnected(IClient* CL)
{
    // csPlayers.Enter			();

    // Game config (all, info includes deleted player now, excludes at the next cl-update)
    NET_Packet P;
    P.B.count = 0;
    P.w_clientID(CL->ID);
    xrClientData* xrCData = (xrClientData*)(CL);
    VERIFY(xrCData);

    if (!xrCData->ps)
        return;

    P.w_stringZ(xrCData->ps->getName());
    P.w_u16(xrCData->ps->GameID);
    P.r_pos = 0;

    ClientID clientID;
    clientID.set(0);

    game->AddDelayedEvent(P, GAME_EVENT_PLAYER_DISCONNECTED, 0, clientID);

    //
    xrS_entities::iterator I = entities.begin(), E = entities.end();
    if (GetClientsCount() > 1 && !CL->flags.bLocal)
    {
        // Migrate entities
        for (; I != E; ++I)
        {
            CSE_Abstract* entity = I->second;
            if (entity->owner == CL)
                PerformMigration(entity, (xrClientData*)CL, SelectBestClientToMigrateTo(entity, TRUE));
        }
    }
    else
    {
        // Destroy entities
        while (!entities.empty())
        {
            CSE_Abstract* entity = entities.begin()->second;
            entity_Destroy(entity);
        }
    }
    // csPlayers.Leave			();

    Server_Client_Check(CL);
}
