#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"

void xrServer::Perform_game_export()
{
    if (!SV_Client)
        return;

    xrClientData* CL = (xrClientData*)SV_Client;
    if (!CL->net_Accepted)
        return;
    
    NET_Packet P;

    P.w_begin(M_SV_CONFIG_GAME);
    game->net_Export_State(P, CL->ID);
    SendTo(CL->ID, P);

    game->sv_force_sync = FALSE;
}

void xrServer::Export_game_type(IClient* CL)
{
    NET_Packet P;
    P.w_begin(M_SV_CONFIG_NEW_CLIENT);
    SendTo(CL->ID, P);
}
