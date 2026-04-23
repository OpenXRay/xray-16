#include "stdafx.h"
#include "IClientStatistic.h"

#pragma warning(push)
#pragma warning(disable:4995)
#if defined(XR_PLATFORM_WINDOWS)
#include <DPlay/dplay8.h>
#endif
#include <steam/steamnetworkingsockets.h>
#pragma warning(pop)

void IClientStatistic::Update(DPN_CONNECTION_INFO& CI)
{
	u32 time_global = TimeGlobal(device_timer);
	if (time_global - dwBaseTime >= 999)
	{
		dwBaseTime = time_global;

		mps_recive = CI.dwMessagesReceived - mps_receive_base;
		mps_receive_base = CI.dwMessagesReceived;

		u32	cur_msend = CI.dwMessagesTransmittedHighPriority + CI.dwMessagesTransmittedNormalPriority + CI.dwMessagesTransmittedLowPriority;
		mps_send = cur_msend - mps_send_base;
		mps_send_base = cur_msend;

		dwBytesSendedPerSec = dwBytesSended;
		dwBytesSended = 0;
		dwBytesReceivedPerSec = dwBytesReceived;
		dwBytesReceived = 0;
	}

	dwRoundTripLatencyMS = CI.dwRoundTripLatencyMS;
	dwThroughputBPS = CI.dwThroughputBPS;
	dwPeakThroughputBPS = CI.dwPeakThroughputBPS;
	dwPacketsDropped = CI.dwPacketsDropped;
	dwPacketsRetried = CI.dwPacketsRetried;
}

void IClientStatistic::Update(SteamNetConnectionRealTimeStatus_t& status)
{
	u32 time_global = TimeGlobal(device_timer);
	if (time_global - dwBaseTime >= 999)
	{
		dwBaseTime = time_global;

		dwBytesSendedPerSec = dwBytesSended; // from other place
		dwBytesSended = 0;
		dwBytesReceivedPerSec = dwBytesReceived; // from other place
		dwBytesReceived = 0;
	}

	dwRoundTripLatencyMS = status.m_nPing;

	qualityLocal = status.m_flConnectionQualityLocal;
	qualityRemote = status.m_flConnectionQualityRemote;

	packetsInPerSec = status.m_flInPacketsPerSec;
	packetsOutPerSec = status.m_flOutPacketsPerSec;

	queueTime = status.m_usecQueueTime;

	sendRateBytesPerSecond = status.m_nSendRateBytesPerSecond;

	pendingReliable = status.m_cbPendingReliable;
	pendingUnreliable = status.m_cbPendingUnreliable;

	sentUnackedReliable = status.m_cbSentUnackedReliable;
}
