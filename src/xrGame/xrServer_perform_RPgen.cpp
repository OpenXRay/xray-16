#include "StdAfx.h"
#include "xrServer.h"

BOOL xrServer::PerformRP(CSE_Abstract* EEE)
{
    /*
    // Get LIST of respawn points
    if (EEE->g_team() >= (int)(Level().Teams.size()))	return FALSE;
    svector<Fvector4,maxRP>&	RP					= Level().Teams[EEE->g_team()].RespawnPoints;
    if (RP.empty())									return FALSE;

    u32	selected	= 0;
    switch (EEE->s_RP)	{
    case 0xFE:	// Use supplied coords
        return TRUE;
    default:	// Use specified RP
        if (EEE->s_RP>=RP.size())	Msg("! ERROR: Can't spawn entity at RespawnPoint #%d.", u32(EEE->s_RP));
        selected = u32(EEE->s_RP);
        break;
    case 0xFF:	// Search for best RP for this entity
    case 0xFD:	// Search for best RP for this entity
        {
            selected	= ::Random.randI	(0,RP.size());
        }
        break;
    }

    // Perform spawn
    Fvector4&			P = Level().Teams[EEE->g_team()].RespawnPoints[selected];
    EEE->o_Position.set	(P.x,P.y,P.z);
    EEE->o_Angle.set	(0,P.w,0);
    */
    return TRUE;
}
