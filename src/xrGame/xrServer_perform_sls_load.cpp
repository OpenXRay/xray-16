#include "StdAfx.h"
#include "xrServer.h"
#include "xrMessages.h"

void xrServer::SLS_Load(IReader& fs)
{
    // Generate spawn+update
    NET_Packet P;
    u16 u_id = 0xffff;
    u32 C;
    for (IReader* F = fs.open_chunk_iterator(C); F; F = fs.open_chunk_iterator(C, F))
    {
        // Spawn
        P.B.count = F->r_u16();
        F->r(P.B.data, P.B.count);
        P.r_begin(u_id);
        R_ASSERT(M_SPAWN == u_id);
        ClientID clientID;
        clientID.set(0);
        Process_spawn(P, clientID);

        // Update
        P.B.count = F->r_u16();
        F->r(P.B.data, P.B.count);
        P.r_begin(u_id);
        R_ASSERT(M_UPDATE == u_id);

        clientID.set(0);
        Process_update(P, clientID);
    }
}
