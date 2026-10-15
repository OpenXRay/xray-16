#pragma once
#include "BaseClient.h"
#include <steam/steamnetworkingsockets.h>

class XRNETSERVER_API SteamNetClient : public BaseClient
{
	friend void ClSteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t *pInfo);
	friend void steam_net_update_client(void* P);

private:
    Lock csConnection;
	ISteamNetworkingSockets*  m_pInterface = nullptr;
	HSteamNetConnection m_hConnection = k_HSteamNetConnection_Invalid;

	xr_string m_user_name = "";
    xr_string m_user_pass = "";
	bool m_bWasConnected = false;
	bool m_bServerClient = false;
	bool m_bGameDescriptionRecieved = false;

public:
	SteamNetClient(CTimer* tm);
	virtual ~SteamNetClient();

private:
	IC bool IsConnectionCreated() const { return m_pInterface != nullptr; }
	IC bool GameDescriptionReceived() const { return m_bGameDescriptionRecieved; }
    void SendClientData();
	void Update();
	void OnSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo);

	void GetIpAddress(SteamNetConnectionInfo_t& info, ip_address& out);

	void PollConnectionStateChanges();
	void PollIncomingMessages();
	bool SetIdentity(SteamNetworkingIdentity& identity, ClientConnectionOptions& opt) const;

protected:
	virtual bool IsConnectionInit() override { return m_pInterface != nullptr; }

	virtual bool CreateConnection(ClientConnectionOptions& opt) override;
	virtual void DestroyConnection() override;
	virtual	void SendTo_LL(void* data, u32 size, u32 dwFlags = xr_send_GUARANTEED, u32 dwTimeout = 0) override;

	virtual bool GetPendingMessagesCount(DWORD& dwPending) override;
	virtual bool SendPingMessage(MSYS_PING& clPing) override;

public:
	virtual	bool GetServerAddress(ip_address& pAddress, DWORD* pPort)  override;

	virtual bool HasSessionName() { return GameDescriptionReceived(); }
	virtual LPCSTR net_SessionName() const override { return m_game_description.map_name; }

	// statistic
	virtual	void UpdateStatistic()  override;
};

