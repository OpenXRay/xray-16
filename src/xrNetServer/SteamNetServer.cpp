#include "stdafx.h"
#include "SteamNetServer.h"
#include <steam/isteamnetworkingutils.h>

SteamNetServer* s_pCallbackInstance = nullptr;

void SvSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo)
{
	if (s_pCallbackInstance)
	{
		s_pCallbackInstance->OnSteamNetConnectionStatusChanged(pInfo);
	}
}

void steam_net_update_server(void* P)
{
	Msg("- [SteamNetServer] Thread for steam network server is started");
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	SteamNetServer*	C = (SteamNetServer*)P;
	C->Update();
}

SteamNetServer::SteamNetServer(CTimer* timer, BOOL	dedicated) : BaseServer(timer, dedicated)
#ifdef PROFILE_CRITICAL_SECTIONS
	, csConnection(MUTEX_PROFILE_ID(SteamNetServer::csConnection))
#endif
{
	m_players.reserve((dedicated) ? GetMaxPlayers() + 1 : GetMaxPlayers());
	m_server_password.reserve(64);
	m_pending_clients.clear();
}

SteamNetServer::~SteamNetServer()
{
}

void SteamNetServer::_SendTo_LL(ClientID ID, void * data, u32 size, u32 dwFlags, u32 dwTimeout)
{
	EResult result = m_pInterface->SendMessageToConnection(ID.value(), data, size, convert_flags_for_steam(dwFlags), nullptr);
	if (result != k_EResultOK)
	{
		Msg("! [SteamNetClient] ERROR: Failed to send net-packet, reason: %d", result);
	}
}

bool SteamNetServer::CreateConnection(GameDescriptionData & game_descr, ServerConnectionOptions & connectOpt)
{
	SteamDatagramErrMsg errMsg;
	SteamNetworkingIdentity identity;
	identity.Clear();
	identity.SetLocalHost();

	if (!GameNetworkingSockets_Init(&identity, errMsg))
	{
		Msg("! [SteamNetServer] GameNetworkingSockets_Init failed.  %s", errMsg);
		return false;
	}

	m_pInterface = SteamNetworkingSockets();

	if (m_pInterface == NULL)
	{
		Msg("! [SteamNetServer] Server interface is NULL");
		return false;
	}

	// SETUP SERVER PORT
	SteamNetworkingIPAddr bindServerAddress;
	bindServerAddress.Clear();
	bindServerAddress.m_port = (uint16)connectOpt.dwServerPort;

	psNET_Port = connectOpt.dwServerPort;

	// SET UPPER LIMIT OF BUFFERED PENDING BYTES TO BE SENT
	SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_SendBufferSize, 1024 * 1024);

	// SETUP CALLBACK
	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SvSteamNetConnectionStatusChangedCallback);

	// CREATE LISTENER
	m_hListenSock = m_pInterface->CreateListenSocketIP(bindServerAddress, 1, &opt);

	if (m_hListenSock == k_HSteamListenSocket_Invalid)
	{
		Msg("! [SteamNetServer] Failed to listen on port %d", bindServerAddress.m_port);
		return false;
	}

	m_hPollGroup = m_pInterface->CreatePollGroup();
	if (m_hPollGroup == k_HSteamNetPollGroup_Invalid)
	{
		Msg("! [SteamNetServer] Failed to create poll group");
		return false;
	}

	// set m_game_description
	CopyMemory(&m_game_description, &game_descr, sizeof(game_descr));

	m_server_password = connectOpt.server_pass;
	m_max_players = (m_bDedicated) ? (connectOpt.dwMaxPlayers + 1) : (connectOpt.dwMaxPlayers);

	Msg("- [SteamNetServer] created on port %d", bindServerAddress.m_port);
    Threading::SpawnThread("snetwork-update-server", [this]
        {
            steam_net_update_server(this);
        });

	return true;
}

void SteamNetServer::DestroyConnection()
{
	// Pavel: Disconnect should not be executed during the callback processing
    ScopeLock lock{ &csConnection };

	m_server_password.clear();

	if (m_pInterface == nullptr)
	{
		return;
	}

	DisconnectAll();

	if (m_hListenSock != k_HSteamListenSocket_Invalid)
	{
		m_pInterface->CloseListenSocket(m_hListenSock);
		m_hListenSock = k_HSteamListenSocket_Invalid;
	}

	if (m_hPollGroup != k_HSteamListenSocket_Invalid)
	{
		m_pInterface->DestroyPollGroup(m_hPollGroup);
		m_hPollGroup = k_HSteamListenSocket_Invalid;
	}

	m_pInterface = nullptr;

	GameNetworkingSockets_Kill();
}

void SteamNetServer::Update()
{
	while (true)
	{
		csConnection.Enter();
		if (!IsConnectionCreated())
		{
			csConnection.Leave();
			return;
		}
		PollIncomingMessages();
		PollConnectionStateChanges();

		csConnection.Leave();
		Sleep(1);
	}
}

void SteamNetServer::PollConnectionStateChanges()
{
	s_pCallbackInstance = this;
	m_pInterface->RunCallbacks();
}

void SteamNetServer::PollIncomingMessages()
{
	while (true)
	{
		if (m_pInterface == nullptr)
			break;

		ISteamNetworkingMessage *pIncomingMsg = nullptr;

		int numMsgs = m_pInterface->ReceiveMessagesOnPollGroup(m_hPollGroup, &pIncomingMsg, 1);
		if (numMsgs <= 0)
		{
			break;
		}

		void*	m_data = pIncomingMsg->m_pData;
		u32	m_size = pIncomingMsg->m_cbSize;
		HSteamNetConnection m_sender = pIncomingMsg->m_conn;

		MSYS_PING* m_ping = (MSYS_PING*)m_data;

        if ((m_size >= 2 * sizeof(u32))
            && m_ping->sign1 == 0x12071980
            && m_ping->sign2 == 0x26111975)
		{
			// ping - save server time and reply
			m_ping->dwTime_Server = TimerAsync(device_timer);
			ClientID ID; ID.set(m_sender);
			BaseServer::SendTo_Buf(ID, m_data, m_size, net_flags(FALSE, FALSE, TRUE, TRUE));
		}
		else if (m_size == sizeof(MSYS_CLIENT_DATA) && m_ping->sign1 == 0x02281488 && m_ping->sign2 == 0x01488228)
		{ // client data message
			OnClientDataReceived(pIncomingMsg->m_conn, pIncomingMsg->m_identityPeer, (MSYS_CLIENT_DATA*)m_data);
		}
		else
		{
			MultipacketReciever::RecievePacket(m_data, m_size, m_sender);
		}
		pIncomingMsg->Release();
	}
}

void SteamNetServer::ProcessConnection(SteamNetConnectionStatusChangedCallback_t * pInfo)
{
	if (std::find(m_players.begin(), m_players.end(), pInfo->m_hConn) != m_players.end())
	{
		Msg("[SteamNetServer] Connection already exist");
		return;
	}

	// Check to max players
	if (m_players.size() >= m_max_players)
	{
		m_pInterface->CloseConnection(pInfo->m_hConn, ESessionFull, nullptr, false);
		return;
	}
	// Add to connection list
	m_players.push_back(pInfo->m_hConn);

	ip_address ip;
	string64 ip_str;
	GetIpAddress(pInfo->m_info, ip);
	ip.to_buf(ip_str, sizeof(string64));

	Msg("[SteamNetServer] Client connecting %s", ip_str);

	// Check banned client
	if (GetBannedClient(ip))
	{
		Msg("[SteamNetServer] Close connection. Player is banned [%s]", ip_str);
		CloseConnection(pInfo->m_hConn, EPlayerBanned);
		return;
	}

	// First connected client is SV_Client so if it is NULL then this server client tries to connect ;)
	bool bServerClient = !SV_Client && pInfo->m_info.m_identityRemote.IsLocalHost();

	if (!bServerClient && !m_ip_filter.is_ip_present(ip.m_data.data))
	{
		Msg("[SteamNetServer] Close connection. Ip address is not present [%s]", ip_str);
		CloseConnection(pInfo->m_hConn, EUnknownReason);
		return;
	}

	// Check server password
	if (!bServerClient)
	{
		LPCSTR pServerPass = pInfo->m_info.m_identityRemote.GetGenericString();
		if (pServerPass != NULL && xr_strcmp(m_server_password.c_str(), pServerPass) != 0)
		{
			Msg("[SteamNetServer] Close connection. Incorrect server password");
			CloseConnection(pInfo->m_hConn, EInvalidPassword);
			return;
		}
	}

	// Try to accept the connection.
	if (m_pInterface->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
	{
		// This could fail.  If the remote host tried to connect, but then
		// disconnected, the connection may already be half closed.  Just
		// destroy whatever we have on our side.
		Msg("[SteamNetServer] Can't accept connection.  (It was already closed?)");
		CloseConnection(pInfo->m_hConn, EUnknownReason);
		return;
	}

	// Assign the poll group
	if (!m_pInterface->SetConnectionPollGroup(pInfo->m_hConn, m_hPollGroup))
	{
		Msg("[SteamNetServer] Close connection. Failed to set poll group");
		CloseConnection(pInfo->m_hConn, EUnknownReason);
		return;
	}
}

void SteamNetServer::AddPendingClient(SClientConnectData & cl_data)
{
	auto pending_client_it = std::find_if(m_pending_clients.cbegin(), m_pending_clients.cend(),
		[&](const auto &data) {
			return data.clientID == cl_data.clientID;
		}
	);
	if (pending_client_it == m_pending_clients.cend())
	{
		m_pending_clients.push_back(cl_data);
	}
}

void SteamNetServer::HandlePendingClients()
{
	if (m_pending_clients.empty())
		return;

	for (auto &cl_data : m_pending_clients)
		FinishConnection(cl_data);

	m_pending_clients.clear();
}

void SteamNetServer::OnClientDataReceived(HSteamNetConnection connection, SteamNetworkingIdentity &identity, MSYS_CLIENT_DATA* data)
{
	SClientConnectData cl_data;
	cl_data.clientID.set(connection); // set clientId
	cl_data.process_id = data->process_id;

	if (identity.IsLocalHost() && data->process_id == GetCurrentProcessId())
	{ // if server client
		xr_strcpy(cl_data.name, "ServerAdmin");
		xr_strcpy(cl_data.pass, "pass");

		FinishConnection(cl_data);
		m_bServerClientConnected = true;
		Msg("- [SteamNetServer] server client connected");

		HandlePendingClients(); // Finish connection for all pending clients
	}
	else
	{
		xr_strcpy(cl_data.name, data->name);
		xr_strcpy(cl_data.pass, data->pass);

		if (m_bServerClientConnected)
			FinishConnection(cl_data);
		else
			AddPendingClient(cl_data);
	}
}

void SteamNetServer::FinishConnection(SClientConnectData &cl_data)
{
	// register client
	new_client(&cl_data);

	// send game description to client
	MSYS_GAME_DESCRIPTION sys_gd;
	sys_gd.sign1 = 0x02281488;
	sys_gd.sign2 = 0x01488228;
	CopyMemory(&sys_gd.data, &m_game_description, sizeof(m_game_description));

	_SendTo_LL(cl_data.clientID, &sys_gd, sizeof(sys_gd), net_flags(TRUE, TRUE, FALSE, FALSE));
}


void SteamNetServer::OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t * pInfo)
{
	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_Connecting:
		ProcessConnection(pInfo);
		break;

	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
		// Ignore if they were not previously connected.  (If they disconnected
		// before we accepted the connection.)
		if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
		{
			if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
			{
				Msg("[SteamNetServer] Connection %s problem detected locally, reason %d: %s\n",
					pInfo->m_info.m_szConnectionDescription,
					pInfo->m_info.m_eEndReason,
					pInfo->m_info.m_szEndDebug
				);
			}
			else
			{
				Msg("[SteamNetServer] Connection %s closed by peer, reason %d: %s\n",
					pInfo->m_info.m_szConnectionDescription,
					pInfo->m_info.m_eEndReason,
					pInfo->m_info.m_szEndDebug
				);
			}

			CloseConnection(pInfo->m_hConn, EUnknownReason);
			DestroyCleint(pInfo->m_hConn);
		}
		break;

	case k_ESteamNetworkingConnectionState_None:
		// NOTE: We will get callbacks here when we destroy connections.  You can ignore these.
		break;

	case k_ESteamNetworkingConnectionState_Connected:
		// NOTE: We will get a callback immediately after accepting the connection.
		// Since we are the server, we can ignore this, it's not news to us.
		break;

	default:
		Msg("! [SteamNetServer] unknown steam connection state %d", pInfo->m_info.m_eState);
		break;
	}
}

void SteamNetServer::DisconnectAll()
{
	for (auto &connection : m_players)
	{
		m_pInterface->CloseConnection(connection, EServerShutdown, nullptr, false);
		DestroyCleint(connection);
	}
	m_players.clear();

	for (auto &cl_data : m_pending_clients)
	{
		m_pInterface->CloseConnection(cl_data.clientID.value(), EServerShutdown, nullptr, false);
	}
	m_pending_clients.clear();
}

bool SteamNetServer::DisconnectClient(IClient * C, LPCSTR Reason)
{
	if (!C) return false;

	CloseConnection(C->ID.value(), EDetailedReason, Reason);
	DestroyCleint(C->ID.value());
	return true;
}

void SteamNetServer::CloseConnection(HSteamNetConnection connection, enmDisconnectReason nReason, LPCSTR sReason)
{
	auto player_it = std::find(m_players.cbegin(), m_players.cend(), connection);
	if (player_it != m_players.cend())
	{
		m_players.erase(player_it);
		m_pInterface->CloseConnection(connection, nReason, sReason, false);
	}

	if (!m_pending_clients.empty())
	{
		auto pending_client_it = std::find_if(m_pending_clients.cbegin(), m_pending_clients.cend(),
			[&](const auto &cl_data) {
				return cl_data.clientID == connection;
			}
		);
		if (pending_client_it != m_pending_clients.cend())
		{
			m_pending_clients.erase(pending_client_it);
			m_pInterface->CloseConnection(connection, nReason, sReason, false);
		}
	}
}

void SteamNetServer::DestroyCleint(ClientID clientId)
{
	IClient* tmp_client = net_players.GetFoundClient(
		ClientIdSearchPredicate(clientId)
	);

	if (tmp_client)
	{
		tmp_client->flags.bConnected = FALSE;
		tmp_client->flags.bReconnect = FALSE;
		OnCL_Disconnected(tmp_client);
		// real destroy
		client_Destroy(tmp_client);
	}
}

void SteamNetServer::GetIpAddress(SteamNetConnectionInfo_t & info, ip_address & out)
{
	string64 ip_str;
	info.m_addrRemote.ToString(ip_str, sizeof(string64), false);
	out.set(ip_str);
}

bool SteamNetServer::GetClientAddress(ClientID ID, ip_address& address, DWORD* pPort)
{
	SteamNetConnectionInfo_t info;
	if (!m_pInterface->GetConnectionInfo(ID.value(), &info))
	{
		return false;
	}

	if (pPort != NULL)
	{
		*pPort = info.m_addrRemote.m_port;
	}

	GetIpAddress(info, address);

	return true;
}

bool SteamNetServer::GetClientPendingMessagesCount(ClientID ID, DWORD & dwPending)
{
	R_ASSERT(m_pInterface);

    SteamNetConnectionRealTimeStatus_t status;
	if (m_pInterface->GetConnectionRealTimeStatus(ID.value(), &status, 0, nullptr))
	{
		dwPending = status.m_cbPendingReliable + status.m_cbPendingUnreliable;
		return true;
	}

	return false;
}

void SteamNetServer::UpdateClientStatistic(IClient* C)
{
    SteamNetConnectionRealTimeStatus_t status;
	if (!m_pInterface->GetConnectionRealTimeStatus(C->ID.value(), &status, 0, nullptr))
	{
		return;
	}
	C->stats.Update(status);
}
