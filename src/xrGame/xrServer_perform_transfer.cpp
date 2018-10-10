#include "StdAfx.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "xrServer_Objects.h"

void xrServer::Perform_transfer(
    NET_Packet& PR, NET_Packet& PT, CSE_Abstract* what, CSE_Abstract* from, CSE_Abstract* to)
{
    // Sanity check
    R_ASSERT(what && from && to);
    R_ASSERT(from != to);
    R_ASSERT(what->ID_Parent == from->ID);
    u32 time = Device.dwTimeGlobal;

    // 1. Perform migration if need it
    if (from->owner != to->owner)
        PerformMigration(what, from->owner, to->owner);
    // Log						("B");

    // 2. Detach "FROM"
    xr_vector<u16>& C = from->children;
    xr_vector<u16>::iterator c = std::find(C.begin(), C.end(), what->ID);
    R_ASSERT(C.end() != c);
    C.erase(c);
    PR.w_begin(M_EVENT);
    PR.w_u32(time);
    PR.w_u16(GE_OWNERSHIP_REJECT);
    PR.w_u16(from->ID);
    PR.w_u16(what->ID);

    // 3. Attach "TO"
    what->ID_Parent = to->ID;
    to->children.push_back(what->ID);
    PT.w_begin(M_EVENT);
    PT.w_u32(time + 1);
    PT.w_u16(GE_OWNERSHIP_TAKE);
    PT.w_u16(to->ID);
    PT.w_u16(what->ID);
}

void xrServer::Perform_reject(CSE_Abstract* what, CSE_Abstract* from, int delta)
{
    R_ASSERT(what && from);
    R_ASSERT(what->ID_Parent == from->ID);

    NET_Packet P;
    u32 time = Device.dwTimeGlobal - delta;

    P.w_begin(M_EVENT);
    P.w_u32(time);
    P.w_u16(GE_OWNERSHIP_REJECT);
    P.w_u16(from->ID);
    P.w_u16(what->ID);
    P.w_u8(1);

    Process_event_reject(P, BroadcastCID, time, from->ID, what->ID);
}
