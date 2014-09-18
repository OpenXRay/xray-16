#pragma once
#ifdef XR_NETSERVER_EXPORTS
	#define XRNETSERVER_API __declspec(dllexport)
#else
	#define XRNETSERVER_API __declspec(dllimport)

	#ifndef _EDITOR
		#pragma comment(lib,	"xrNetServer"	)
    #endif
#endif

#include "../xrCore/net_utils.h"
#include <dplay/dplay8.h>
#include "net_messages.h"



#include "net_compressor.h"

XRNETSERVER_API extern ClientID BroadcastCID;

XRNETSERVER_API extern Flags32	psNET_Flags;
XRNETSERVER_API extern int		psNET_ClientUpdate;
XRNETSERVER_API extern int		get_psNET_ClientUpdate();
XRNETSERVER_API extern int		psNET_ClientPending;
XRNETSERVER_API extern char		psNET_Name[];
XRNETSERVER_API extern int		psNET_ServerUpdate;
XRNETSERVER_API extern int		get_psNET_ServerUpdate();
XRNETSERVER_API extern int		psNET_ServerPending;

XRNETSERVER_API extern BOOL		psNET_direct_connect;

enum	{
	NETFLAG_MINIMIZEUPDATES		= (1<<0),
	NETFLAG_DBG_DUMPSIZE		= (1<<1),
	NETFLAG_LOG_SV_PACKETS		= (1<<2),
	NETFLAG_LOG_CL_PACKETS		= (1<<3),
};

IC u32 TimeGlobal	(CTimer* timer)	{ return timer->GetElapsed_ms();	}
IC u32 TimerAsync	(CTimer* timer) { return TimeGlobal	(timer);		}

class XRNETSERVER_API IClientStatistic
{
	DPN_CONNECTION_INFO	ci_last;
	u32					mps_recive, mps_receive_base;
	u32					mps_send,	mps_send_base;
	u32					dwBaseTime;
	CTimer*				device_timer;
public:
			IClientStatistic	(CTimer* timer){ ZeroMemory(this,sizeof(*this)); device_timer=timer; dwBaseTime=TimeGlobal(device_timer); }

	void	Update				(DPN_CONNECTION_INFO& CI);

	IC u32	getPing				()	{ return ci_last.dwRoundTripLatencyMS;	}
	IC u32	getBPS				()	{ return ci_last.dwThroughputBPS;		}
	IC u32	getPeakBPS			()	{ return ci_last.dwPeakThroughputBPS;	}
	IC u32	getDroppedCount		()	{ return ci_last.dwPacketsDropped;		}
	IC u32	getRetriedCount		()	{ return ci_last.dwPacketsRetried;		}
	IC u32	getMPS_Receive		()  { return mps_recive;	}
	IC u32	getMPS_Send			()	{ return mps_send;		}
	IC u32	getReceivedPerSec	()	{ return dwBytesReceivedPerSec; }
	IC u32	getSendedPerSec		()	{ return dwBytesSendedPerSec; }
	

	IC void	Clear				()	{ CTimer* timer = device_timer; ZeroMemory(this,sizeof(*this)); device_timer=timer; dwBaseTime=TimeGlobal(device_timer); }

	//-----------------------------------------------------------------------
	u32		dwTimesBlocked;
	
	u32		dwBytesSended;
	u32		dwBytesSendedPerSec;
	
	u32		dwBytesReceived;
	u32		dwBytesReceivedPerSec;

};

