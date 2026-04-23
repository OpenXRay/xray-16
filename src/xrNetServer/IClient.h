#pragma once
#include "IClientStatistic.h"
#include "NET_Common.h"
#include "ip_address.h"

class BaseServer;

class XRNETSERVER_API IClient: public MultipacketSender
{
public:
	struct Flags
	{
		u32		bLocal : 1;
		u32		bConnected : 1;
		u32		bReconnect : 1;
		u32		bVerified : 1;
	};

	IClient(CTimer* timer);
	virtual             ~IClient();

	IClientStatistic	stats;

	ClientID			ID;
	string128			m_guid;
	shared_str			name;
	shared_str			pass;

	Flags				flags;	// local/host/normal
	u32					dwTime_LastUpdate;

	ip_address			m_cAddress;
	DWORD				m_dwPort;
	u32					process_id;

	BaseServer*        server;

private:

	virtual void    _SendTo_LL(const void* data, u32 size, u32 flags, u32 timeout);
};


IC bool operator== (IClient const* pClient, ClientID const& ID) { return pClient->ID == ID; }
