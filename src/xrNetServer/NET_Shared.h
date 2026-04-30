#pragma once

#include "xrCore/xr_types.h"
#include "xrCore/_flags.h"
#include "xrCore/client_id.h"
#include "xrCore/FTimer.h"

#ifdef XRAY_STATIC_BUILD
#   define XRNETSERVER_API
#else
#   ifdef XR_NETSERVER_EXPORTS
#      define XRNETSERVER_API XR_EXPORT
#   else
#      define XRNETSERVER_API XR_IMPORT
#   endif
#endif

#include "NET_Compressor.h"

// #define USE_DIRECT_PLAY

IC bool UseDirectPlay()
{
#ifdef USE_DIRECT_PLAY
	return true;
#else
	return false;
#endif // USE_DIRECT_PLAY
}

XRNETSERVER_API extern ClientID BroadcastCID;

XRNETSERVER_API extern Flags32 psNET_Flags;
XRNETSERVER_API extern int psNET_ClientUpdate;
XRNETSERVER_API extern int psNET_ClientPending;
XRNETSERVER_API extern char psNET_Name[];
XRNETSERVER_API extern int psNET_ServerUpdate;
XRNETSERVER_API extern int psNET_ServerPending;

XRNETSERVER_API extern bool psNET_direct_connect;

enum
{
    NETFLAG_MINIMIZEUPDATES = 1 << 0,
    NETFLAG_DBG_DUMPSIZE = 1 << 1,
    NETFLAG_LOG_SV_PACKETS = 1 << 2,
    NETFLAG_LOG_CL_PACKETS = 1 << 3,
};

IC u32 TimeGlobal(CTimer* timer) { return timer->GetElapsed_ms(); }
IC u32 TimerAsync(CTimer* timer) { return TimeGlobal(timer); }

