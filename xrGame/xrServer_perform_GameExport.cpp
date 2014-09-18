#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"

void xrServer::Perform_game_export	()
{
	struct NetExportToClientFunctor
	{
		xrServer* server_ptr;
		NetExportToClientFunctor(xrServer* server) :
			server_ptr(server)
		{
		}
		void operator()(IClient* client)
		{

			R_ASSERT(server_ptr);
			NET_Packet		P;
			u32				mode				= net_flags(TRUE,TRUE);
			
			xrClientData*	CL	= (xrClientData*)client;
			if (!CL->net_Accepted)
				return;
			P.w_begin							(M_SV_CONFIG_GAME);
			server_ptr->game->net_Export_State	(P,client->ID);
			server_ptr->SendTo					(client->ID,P,mode);
		}
	};
	NetExportToClientFunctor temp_functor(this);
	ForEachClientDoSender(temp_functor);
	game->sv_force_sync	= FALSE;
}

void xrServer::Export_game_type(IClient* CL)
{
	NET_Packet			P;
	u32					mode = net_flags(TRUE,TRUE);
	P.w_begin			(M_SV_CONFIG_NEW_CLIENT);
	P.w_stringZ			(game->type_name() );
	SendTo				(CL->ID,P,mode);
}

