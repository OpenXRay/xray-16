#include "stdafx.h"
#include "game_sv_capture_the_artefact.h"
#include "xrServer.h"
#include "xrMessages.h"

void game_sv_CaptureTheArtefact::OnEvent(NET_Packet & tNetPacket, u16 type, u32 time, ClientID sender )
{
	switch	(type)
	{
	case GAME_EVENT_PLAYER_KILL:
		{
			u16				ID = tNetPacket.r_u16();
			xrClientData*	l_pC = (xrClientData*)get_client(ID);
			if (!l_pC)
				break;

			KillPlayer		(l_pC->ID, l_pC->ps->GameID);
		}break;
	case GAME_EVENT_PLAYER_BUY_FINISHED:
		{
			xrClientData* l_pC = m_server->ID_to_client(sender);
			OnPlayerBuyFinished(l_pC->ID, tNetPacket);
		}break;
	case GAME_EVENT_PLAYER_ENTER_TEAM_BASE:
		{
			u16 pl_id = tNetPacket.r_u16();
			//warning, in editor green team zone has 1 id, blue team zone has id 2
			u8 z_t_id = tNetPacket.r_u8();
			z_t_id--; // :( !!!
			OnObjectEnterTeamBase(pl_id, z_t_id);
		}break;

	case GAME_EVENT_PLAYER_LEAVE_TEAM_BASE:
		{
 			u16 pl_id = tNetPacket.r_u16();
			u8 z_t_id = tNetPacket.r_u8();
			z_t_id--;						// :( !!!
			OnObjectLeaveTeamBase(pl_id, z_t_id);
		}break;
	default:
		inherited::OnEvent(tNetPacket, type, time, sender);
	};//switch
}