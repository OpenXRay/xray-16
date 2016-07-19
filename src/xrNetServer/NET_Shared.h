#pragma once

#include "Common/Platform.hpp"
#include "xrCore/_types.h"

#ifdef XR_NETSERVER_EXPORTS
#define XRNETSERVER_API XR_EXPORT
#else
#define XRNETSERVER_API XR_IMPORT

#ifndef _EDITOR
#pragma comment(lib, "xrNetServer")
#endif
#endif

// XXX: review and delete
//#include "xrCore/net_utils.h"
//#include <dplay/dplay8.h>
//#include "net_messages.h"

#include "net_compressor.h"

XRNETSERVER_API extern ClientID BroadcastCID;

XRNETSERVER_API extern Flags32 psNET_Flags;
XRNETSERVER_API extern int psNET_ClientUpdate;
XRNETSERVER_API extern int get_psNET_ClientUpdate();
XRNETSERVER_API extern int psNET_ClientPending;
XRNETSERVER_API extern char psNET_Name[];
XRNETSERVER_API extern int psNET_ServerUpdate;
XRNETSERVER_API extern int get_psNET_ServerUpdate();
XRNETSERVER_API extern int psNET_ServerPending;

XRNETSERVER_API extern BOOL psNET_direct_connect;

enum
{
    NETFLAG_MINIMIZEUPDATES = (1 << 0),
    NETFLAG_DBG_DUMPSIZE = (1 << 1),
    NETFLAG_LOG_SV_PACKETS = (1 << 2),
    NETFLAG_LOG_CL_PACKETS = (1 << 3),
};

IC u32 TimeGlobal(CTimer* timer) { return timer->GetElapsed_ms(); }
IC u32 TimerAsync(CTimer* timer) { return TimeGlobal(timer); }
// DPlay
extern "C"
{
typedef struct _DPN_CONNECTION_INFO DPN_CONNECTION_INFO;
}

class XRNETSERVER_API IClientStatistic
{
    struct ClientStatisticImpl* m_pimpl;

public:
    IClientStatistic();
    IClientStatistic(CTimer* timer);
    IClientStatistic(const IClientStatistic& rhs); // Required due to probable bug in CLevel::ProcessCompressedUpdate
    ~IClientStatistic();

    void Update(DPN_CONNECTION_INFO& CI);

    u32 getPing() const;
    u32 getBPS() const;
    u32 getPeakBPS() const;
    u32 getDroppedCount() const;
    u32 getRetriedCount() const;
    u32 getMPS_Receive() const;
    u32 getMPS_Send() const;
    u32 getReceivedPerSec() const;
    u32 getSendedPerSec() const;
    IC void Clear();
    //-----------------------------------------------------------------------
    u32 dwTimesBlocked;

    u32 dwBytesSended;
    u32 dwBytesSendedPerSec;

    u32 dwBytesReceived;
    u32 dwBytesReceivedPerSec;
};
