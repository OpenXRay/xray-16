#include "StdAfx.h"
#include "game_sv_teamdeathmatch.h"
#include "xrServer.h"
#include "xrMessages.h"

void game_sv_TeamDeathmatch::OnEvent(NET_Packet& P, u16 type, u32 time, ClientID sender)
{
    switch (type)
    {
    case GAME_EVENT_PLAYER_ENTER_TEAM_BASE:
    {
        u16 pl_id = P.r_u16();
        // warning, in editor green team zone has 1 id, blue team zone has id 2
        u8 z_t_id = P.r_u8() + 1;
        OnObjectEnterTeamBase(pl_id, z_t_id);
    }
    break;
    case GAME_EVENT_PLAYER_LEAVE_TEAM_BASE:
    {
        u16 pl_id = P.r_u16();
        u8 z_t_id = P.r_u8() + 1;
        OnObjectLeaveTeamBase(pl_id, z_t_id);
    }
    break;
    default: { inherited::OnEvent(P, type, time, sender);
    }
    }; // switch (type)
}
