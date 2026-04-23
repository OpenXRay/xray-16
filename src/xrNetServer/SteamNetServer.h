#pragma once
#include "BaseServer.h"
#include "NET_Messages.h"

#pragma warning(push)
#pragma warning(disable:4995)
#include <steam/steamnetworkingsockets.h>
#pragma warning(pop)

enum enmDisconnectReason
{
	EUnknownReason = k_ESteamNetConnectionEnd_App_Min + 1,
	EDetailedReason = k_ESteamNetConnectionEnd_App_Min + 2,
	EInvalidPassword = k_ESteamNetConnectionEnd_App_Min + 3,
	ESessionFull = k_ESteamNetConnectionEnd_App_Min + 4,
	EPlayerBanned = k_ESteamNetConnectionEnd_App_Min + 5,
	EServerShutdown = k_ESteamNetConnectionEnd_App_Min + 6
};

class XRNETSERVER_API SteamNetServer : public BaseServer
{
	friend void steam_net_update_server(void* P);
	friend void SvSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);

private:
	ISteamNetworkingSockets*      m_pInterface = nullptr;
	HSteamListenSocket            m_hListenSock = k_HSteamListenSocket_Invalid;
	HSteamNetPollGroup            m_hPollGroup = k_HSteamListenSocket_Invalid;

	GameDescriptionData           m_game_description;

	Lock             csConnection;
	xr_string                     m_server_password;
	u32														m_max_players = 0;
	bool													m_bServerClientConnected = false;

	// Using in update thread!
	xr_vector<HSteamNetConnection> m_players;
	xr_vector<SClientConnectData> m_pending_clients;

public:
	SteamNetServer(CTimer* timer, BOOL	dedicated);
	virtual ~SteamNetServer();

private:
	IC bool           IsConnectionCreated() const { return m_pInterface != nullptr; }

	void							ProcessConnection(SteamNetConnectionStatusChangedCallback_t * pInfo);
	void							FinishConnection(SClientConnectData &cl_data);

	void							AddPendingClient(SClientConnectData &cl_data);
	void							HandlePendingClients();
	void							OnClientDataReceived(HSteamNetConnection connection, SteamNetworkingIdentity &identity, MSYS_CLIENT_DATA* data);

	// update thread
	void              OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);
	void			        Update();
	void			        PollConnectionStateChanges();
	void			        PollIncomingMessages();

	void			        DisconnectAll();
	void			        CloseConnection(HSteamNetConnection connection, enmDisconnectReason nReason = EUnknownReason, LPCSTR sReason = nullptr);
	void			        DestroyCleint(ClientID clientId);

	void			        GetIpAddress(SteamNetConnectionInfo_t& info, ip_address& out);

protected:
	virtual bool			CreateConnection(GameDescriptionData& game_descr, ServerConnectionOptions& opt) override;
	virtual void			DestroyConnection() override;

	virtual bool      GetClientPendingMessagesCount(ClientID ID, DWORD& dwPending) override;

	virtual void			_SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags = DPNSEND_GUARANTEED, u32 dwTimeout = 0) override;

public:
	virtual void			UpdateClientStatistic(IClient* C) override;

	virtual bool			GetClientAddress(ClientID ID, ip_address& Address, DWORD* pPort = NULL) override;
	virtual bool			DisconnectClient(IClient* C, LPCSTR Reason) override;
};
