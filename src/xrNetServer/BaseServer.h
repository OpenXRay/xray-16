#pragma once
#include "NET_Shared.h"
#include "ip_filter.h"
#include "PlayersMonitor.h"
#include "IClient.h"
#include "ServerConnectionOptions.h"
#include "xrCore/buffer_vector.h"
#include "IClient.h"
#include "xrCommon/xr_deque.h"

class CServerInfo;
class IBannedClient;
struct ip_address;

class XRNETSERVER_API IServerStatistic
{
public:
	void clear()
	{
		bytes_out = bytes_out_real = 0;
		bytes_in = bytes_in_real = 0;

		dwBytesSended = 0;
		dwSendTime = 0;
		dwBytesPerSec = 0;
	}

	u32 bytes_out, bytes_out_real;
	u32 bytes_in, bytes_in_real;

	u32 dwBytesSended;
	u32 dwSendTime;
	u32 dwBytesPerSec;
};

class XRNETSERVER_API BaseServer : public MultipacketReciever
{
public:
	struct ServerMessage
	{
		NET_Packet P;
		ClientID Id;

		ServerMessage(const void* data, u32 data_size, u32 id)
		{
			P.construct(data, data_size);
			Id.set(id);
		}
	};

    struct ClientIdSearchPredicate
    {
        ClientID clientId;
        ClientIdSearchPredicate(ClientID clientIdToSearch) :
            clientId(clientIdToSearch)
        {
        }
        inline bool operator()(IClient* client) const
        {
            return client->ID == clientId;
        }
    };

    struct ToDisconnectFillerFunctor
    {
        BaseServer* m_owner;
        buffer_vector<IClient*>* dest;
        ip_address const* address_to_disconnect;
        ToDisconnectFillerFunctor(BaseServer* owner, buffer_vector<IClient*>* dest_disconnect, ip_address const* address) :
            m_owner(owner), dest(dest_disconnect), address_to_disconnect(address)
        {
        }
        void operator()(IClient* client)
        {
            ip_address tmp_address;
            m_owner->GetClientAddress(client->ID, tmp_address);
            if (*address_to_disconnect == tmp_address)
            {
                dest->push_back(client);
            };
        }
    };

    struct LocalSenderFunctor
    {
        static void FlushBuffer(IClient* client)
        {
            client->MultipacketSender::FlushSendBuffer(0);
        }
    };

    struct ClientExcluderPredicate
    {
        ClientID id_to_exclude;
        ClientExcluderPredicate(ClientID exclude) :
            id_to_exclude(exclude)
        {
        }
        bool operator()(IClient* client)
        {
            if (client->ID == id_to_exclude)
                return false;
            if (!client->flags.bConnected)
                return false;
            return true;
        }
    };

    struct ClientSenderFunctor
    {
        BaseServer* m_owner;
        void* m_data;
        u32 m_size;
        u32 m_dwFlags;
        ClientSenderFunctor(BaseServer* owner, void* data, u32 size, u32 dwFlags) :
            m_owner(owner), m_data(data), m_size(size), m_dwFlags(dwFlags)
        {
        }
        void operator()(IClient* client)
        {
            m_owner->SendTo_LL(client->ID, m_data, m_size, m_dwFlags);
        }
    };
	enum EConnect
	{
		ErrConnect,
		ErrMax,
		ErrNoError = ErrMax,
	};

private:
#ifdef DEBUG
	bool sender_functor_invoked;
#endif

protected:
	xr_vector<IBannedClient*> BannedAddresses;
	ip_filter m_ip_filter;

	IClient* SV_Client;
	PlayersMonitor net_players;

    Lock csMessage;
	Lock csMessagesQueue;
	xr_deque<ServerMessage> m_messagesQueue;

	int psNET_Port;
	bool m_bDedicated;

	shared_str connect_options;

	// statistic
	IServerStatistic stats;
	CTimer* device_timer;

public:
	BaseServer(CTimer* timer, BOOL	Dedicated);
	virtual ~BaseServer();

protected:
	IBannedClient* GetBannedClient(const ip_address& Address);
	void BannedList_Save();
	void BannedList_Load();
	void IpList_Load();
	void IpList_Unload();
	LPCSTR GetBannedListName();

	void UpdateBannedList();

	void ParseConnectionOptions(LPCSTR options, ServerConnectionOptions& out);

	virtual bool CreateConnection(GameDescriptionData& game_descr, ServerConnectionOptions& opt) = 0;
	virtual void DestroyConnection() = 0;

	virtual bool  GetClientPendingMessagesCount(ClientID ID, DWORD& dwPending) = 0;

	virtual void _Recieve(const void* data, u32 data_size, u32 param) override;
	virtual void _SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0) = 0;
	// Pavel: Calls from xrServer (from game thread)
	virtual void ProcessMessagesQueue();
	virtual IClient* new_client(SClientConnectData* cl_data) = 0;

public:
	IC int GetPort() const { return psNET_Port; };
	IC u32 GetMaxPlayers() const { return 32; }

	EConnect Connect(LPCSTR options, GameDescriptionData & game_descr);
	void Disconnect();

	IClient* ID_to_client(ClientID ID, bool ScanAll = false);

	bool IsPlayerIPDenied(u32 ip_address);
	void Print_Banned_Addreses();
	virtual void BanClient(IClient* C, u32 BanTime);
	virtual void BanAddress(const ip_address& Address, u32 BanTime);
	virtual void UnBanAddress(const ip_address& Address);

	virtual void OnCL_Connected(IClient* C) = 0;
	virtual void OnCL_Disconnected(IClient* C) = 0;

	bool HasBandwidth(IClient* C);

	// statistic
	const IServerStatistic*	GetStatistic() { return &stats; }
	void ClearStatistic();
	virtual void UpdateClientStatistic(IClient* C) = 0;

	// disconnect
	virtual bool DisconnectClient(IClient* C, LPCSTR Reason) = 0;
	virtual bool DisconnectAddress(const ip_address& Address, LPCSTR reason);

	// send
	virtual void SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	virtual void SendTo_Buf(ClientID ID, void* data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	virtual void Flush_Clients_Buffers();

	void SendTo(ClientID ID, NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0);
	void SendBroadcast_LL(ClientID exclude, void* data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED);
	virtual void SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags = DPNSEND_GUARANTEED);


	IC const shared_str& GetConnectOptions() const { return connect_options; }

	IC u32 GetClientsCount() { return net_players.ClientsCount(); };
	IC IClient* GetServerClient() { return SV_Client; };
	IC IClient* GetClientByID(ClientID clientId) { return net_players.GetFoundClient(ClientIdSearchPredicate(clientId)); };

	virtual bool GetClientAddress(ClientID ID, ip_address& Address, DWORD* pPort = NULL) = 0;

	// extended functionality
	virtual bool OnCL_QueryHost() { return true; };
	virtual u32 OnMessage(NET_Packet& P, ClientID sender) { return 0; };	// Non-Zero means broadcasting with "flags" as returned

	virtual IClient* client_Create() = 0; // create client info
	virtual void client_Replicate() = 0; // replicate current state to client
	virtual void client_Destroy(IClient* C) = 0; // destroy client info

	virtual bool Check_ServerAccess(IClient* CL, string512& reason) { return true; }
	virtual void Assign_ServerType(string512& res) {};
	virtual void GetServerInfo(CServerInfo* si) {};

	template<typename SearchPredicate>
	IClient* FindClient(SearchPredicate const & predicate) { return net_players.GetFoundClient(predicate); }
	template<typename ActionFunctor>
	void ForEachClientDo(ActionFunctor & action) { net_players.ForEachClientDo(action); }
	template<typename SenderFunctor>
	void ForEachClientDoSender(SenderFunctor & action) {
		csMessage.Enter();
#ifdef DEBUG
		sender_functor_invoked = true;
#endif //#ifdef DEBUG
		net_players.ForEachClientDo(action);
#ifdef DEBUG
		sender_functor_invoked = false;
#endif //#ifdef DEBUG
		csMessage.Leave();
	}

#ifdef DEBUG
	bool IsPlayersMonitorLockedByMe() const { return net_players.IsCurrentThreadIteratingOnClients() && !sender_functor_invoked; };
#endif
};

