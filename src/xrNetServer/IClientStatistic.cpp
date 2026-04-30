#include "stdafx.h"
#include "IClientStatistic.h"

#include <steam/steamnetworkingsockets.h>

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
