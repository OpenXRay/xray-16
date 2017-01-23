#include "stdafx.h"
#include "game_sv_deathmatch.h"
#include "xrServer.h"
#include "xrMessages.h"

void	game_sv_Deathmatch::OnEvent (NET_Packet &P, u16 type, u32 time, ClientID sender )
{

	switch	(type)
	{
	case GAME_EVENT_PLAYER_KILL: //dm only  (g_kill)
		{
			u16 ID = P.r_u16();
			xrClientData *l_pC = (xrClientData*)get_client(ID);
			if (!l_pC) break;			
			KillPlayer(l_pC->ID, l_pC->ps->GameID);
		}break;


	case GAME_EVENT_PLAYER_BUY_FINISHED: // dm only
		{
			xrClientData *l_pC = m_server->ID_to_client(sender);
#ifdef DEBUG
			Msg("--- On player [%d] buy finishing...", l_pC->ID);
#endif // #ifdef DEBUG
			OnPlayerBuyFinished(l_pC->ID, P);
		}break;

	default:
		inherited::OnEvent(P, type, time, sender);
	};//switch
}


