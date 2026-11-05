#pragma once
#include "NET_Shared.h"
#include "NET_Common.h"
#include "ClientConnectionOptions.h"
#include "IClientStatistic.h"
#include "INetQueue.h"
#include "NET_Messages.h"

struct ip_address;

const u32 syncQueueSize = 512;
const int syncSamples = 256;

class XRNETSERVER_API syncQueue
{
	u32 table[syncQueueSize];
	u32 write;
	u32 count;

public:
	syncQueue() { clear(); }

	void push(u32 value)
	{
		table[write++] = value;
		if (write == syncQueueSize)
			write = 0;

		if (count <= syncQueueSize)
			count++;
	}

	u32* begin() { return table; }
	u32* end() { return table + count; }
	u32 size() const { return count; }

	void clear()
	{
		write = 0;
		count = 0;
	}
};

class XRNETSERVER_API BaseClient : public MultipacketReciever, private MultipacketSender
{
	friend void client_sync_thread(void* P);

public:
	enum ConnectionState
	{
		EnmConnectionFails = 0,
		EnmConnectionWait = -1,
		EnmConnectionCompleted = 1
	};

private:
	ClientID net_ClientID;

protected:
	syncQueue net_DeltaArray;

	u32 net_Time_LastUpdate;
	s32 net_TimeDelta;
	s32 net_TimeDelta_Calculated;
	s32 net_TimeDelta_User;

	GameDescriptionData m_game_description;
	CTimer* device_timer;

	ConnectionState	net_Connected;
	bool net_Syncronised = FALSE;
	bool net_Disconnected = TRUE;

	INetQueue net_Queue;
	IClientStatistic net_Statistic;

public:
	BaseClient(CTimer* tm);
	virtual ~BaseClient();

private:
	virtual void _Recieve(const void* data, u32 data_size, u32 param);
	virtual void _SendTo_LL(const void* data, u32 size, u32 flags, u32 timeout);

protected:
	virtual bool IsConnectionInit() = 0;

	void SetClientID(ClientID const & local_client) { net_ClientID = local_client; };

	void ParseConnectionOptions(LPCSTR options, ClientConnectionOptions& out);
	virtual bool CreateConnection(ClientConnectionOptions& opt) = 0;
	virtual void DestroyConnection() = 0;

	bool Sync_Thread();
	void Sync_Average();
	virtual bool GetPendingMessagesCount(DWORD& dwPending) = 0;
	virtual bool SendPingMessage(MSYS_PING& clPing) = 0;

	virtual	void SendTo_LL(void* data, u32 size, u32 dwFlags = xr_send_GUARANTEED, u32 dwTimeout = 0) = 0;

public:
	bool Connect(LPCSTR options);
	void Disconnect();

	ClientID const& GetClientID() { return net_ClientID; };

	bool net_isCompleted_Connect() const { return net_Connected == EnmConnectionCompleted; }
	bool net_isFails_Connect() const { return net_Connected == EnmConnectionFails; }
	bool net_isCompleted_Sync() const { return net_Syncronised; }
	bool net_isDisconnected() const { return net_Disconnected; }
	IC GameDescriptionData const & get_net_DescriptionData() const { return m_game_description; }

	virtual bool HasSessionName() { return false; }
	virtual LPCSTR net_SessionName() const = 0;

	bool net_HasBandwidth();

	bool net_IsSyncronised() const { return net_Syncronised; };
	void net_Syncronize();

	// receive
	IC void StartProcessQueue() { net_Queue.Lock(); };
	IC NET_Packet* net_msg_Retreive() { return net_Queue.Retreive(); };
	IC void net_msg_Release() { net_Queue.Release(); };
	IC void EndProcessQueue() { net_Queue.Unlock(); };

	// send
	virtual	void Send(NET_Packet& P, u32 dwFlags = xr_send_GUARANTEED, u32 dwTimeout = 0);
	virtual void Flush_Send_Buffer();

	virtual void OnMessage(void* data, u32 size);
	virtual void OnInvalidHost() {};
	virtual void OnInvalidPassword() {};
	virtual void OnSessionFull() {};
	virtual void OnConnectRejected() {};


	virtual	LPCSTR GetMsgId2Name(u16 ID) { return ""; }
	virtual void OnSessionTerminate(LPCSTR reason) {};

	virtual	bool GetServerAddress(ip_address& pAddress, DWORD* pPort) = 0;

	// time management
	IC u32 timeServer() { return TimeGlobal(device_timer) + net_TimeDelta + net_TimeDelta_User; }
	IC u32 timeServer_Async() { return TimerAsync(device_timer) + net_TimeDelta + net_TimeDelta_User; }
	IC u32 timeServer_Delta() { return net_TimeDelta; }
	IC void timeServer_UserDelta(s32 d) { net_TimeDelta_User = d; }

	// unused
	//IC void timeServer_Correct(u32 sv_time, u32 cl_time);

	// statistic
	void ClearStatistic() { net_Statistic.Clear(); };
	IClientStatistic& GetStatistic() { return  net_Statistic; }
	virtual	void UpdateStatistic() = 0;
};

