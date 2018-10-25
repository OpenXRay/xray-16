#pragma once

#include "net_shared.h"
#include "NET_PlayersMonitor.h"

struct SClientConnectData
{
    ClientID clientID;
    string64 name;
    string64 pass;
    u32 process_id;

    SClientConnectData()
    {
        name[0] = pass[0] = 0;
        process_id = 0;
    }
};

// -----------------------------------------------------

class IPureServer;

struct XRNETSERVER_API ip_address
{
    union
    {
        struct
        {
            u8 a1;
            u8 a2;
            u8 a3;
            u8 a4;
        };

        u32 data;
    } m_data;

    void set(pcstr src_string);
    xr_string to_string() const;

    bool operator==(const ip_address& other) const
    {
        return m_data.data == other.m_data.data ||
            m_data.a1 == other.m_data.a1 &&
            m_data.a2 == other.m_data.a2 &&
            m_data.a3 == other.m_data.a3 &&
            m_data.a4 == 0;
    }
};

class XRNETSERVER_API IClient
{
public:
    struct Flags
    {
        u32 bLocal : 1;
        u32 bConnected : 1;
        u32 bReconnect : 1;
        u32 bVerified : 1;
    };

    IClient(CTimer* timer);
    virtual ~IClient();

    IClientStatistic stats;

    ClientID ID;
    shared_str name;
    shared_str pass;

    Flags flags; // local/host/normal
    u32 dwTime_LastUpdate;

    ip_address m_cAddress;
    DWORD m_dwPort;
    u32 process_id;

    IPureServer* server;
};

IC bool operator==(IClient const* pClient, ClientID const& ID) { return pClient->ID == ID; }

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

class XRNETSERVER_API IBannedClient
{
public:
    ip_address HAddr;
    time_t BanTime;

    IBannedClient()
    {
        HAddr.m_data.data = 0;
        BanTime = 0;
    }
    void Load(CInifile& ini, const shared_str& sect);
    void Save(CInifile& ini);

    xr_string BannedTimeTo() const;
};

//==============================================================================

struct ClientIdSearchPredicate
{
    ClientID clientId;
    ClientIdSearchPredicate(ClientID clientIdToSearch) : clientId(clientIdToSearch) {}
    bool operator()(IClient* client) const { return client->ID == clientId; }
};

class CSE_Abstract;
class CServerInfo;
class IServerGameState;

// DPlay
extern "C"
{
    struct IDirectPlay8Server;
    struct IDirectPlay8Address;
}

class XRNETSERVER_API IPureServer
{
public:
    enum EConnect
    {
        ErrConnect,
        ErrMax,
        ErrNoError = ErrMax,
    };

protected:
    shared_str connect_options;
    IDirectPlay8Server* NET;

    PlayersMonitor net_players;
    IClient* SV_Client;

    int psNET_Port;

    xr_vector<IBannedClient*> BannedAddresses;

    //
    Lock csMessage;

    // Statistic
    IServerStatistic stats;
    CTimer* device_timer;
    bool m_bDedicated;

    IClient* ID_to_client(ClientID ID, bool ScanAll = false);

    virtual IClient* new_client(SClientConnectData* cl_data) = 0;

public:
    IPureServer(CTimer* timer, bool Dedicated = false);
    virtual ~IPureServer();

    virtual EConnect Connect(pcstr session_name, GameDescriptionData& game_descr);
    virtual void Disconnect();

    // send
    virtual void SendTo_LL(ClientID ID, void* data, u32 size, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/, u32 dwTimeout = 0);

    void SendTo(ClientID ID, NET_Packet& P, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/, u32 dwTimeout = 0);
    void SendBroadcast_LL(ClientID exclude, void* data, u32 size, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/);
    virtual void SendBroadcast(ClientID exclude, NET_Packet& P, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/);

    // statistic
    const IServerStatistic* GetStatistic() const { return &stats; }

    // extended functionality
    virtual u32 OnMessage(NET_Packet& P, ClientID sender); // Non-Zero means broadcasting with "flags" as returned
    virtual IClient* client_Create() = 0; // create client info
    virtual void client_Replicate() = 0; // replicate current state to client
    virtual void client_Destroy(IClient* C) = 0; // destroy client info

    int GetPort() const { return psNET_Port; }

    u32 GetClientsCount() { return net_players.ClientsCount(); };
    IClient* GetServerClient() const { return SV_Client; };

    template <typename SearchPredicate>
    IClient* FindClient(SearchPredicate const& predicate)
    {
        return net_players.GetFoundClient(predicate);
    }

    template <typename ActionFunctor>
    void ForEachClientDo(ActionFunctor& action)
    {
        net_players.ForEachClientDo(action);
    }

    template <typename SenderFunctor>
    void ForEachClientDoSender(SenderFunctor& action)
    {
        csMessage.Enter();
#ifdef DEBUG
        sender_functor_invoked = true;
#endif
        net_players.ForEachClientDo(action);
#ifdef DEBUG
        sender_functor_invoked = false;
#endif
        csMessage.Leave();
    }

#ifdef DEBUG
    bool IsPlayersMonitorLockedByMe() const
    {
        return net_players.IsCurrentThreadIteratingOnClients() && !sender_functor_invoked;
    };
#endif

    IClient* GetClientByID(const ClientID& clientId)
    {
        return net_players.GetFoundClient(ClientIdSearchPredicate(clientId));
    }

    const shared_str& GetConnectOptions() const { return connect_options; }
    virtual IServerGameState* GetGameState() = 0;
    virtual u16 PerformIDgen(u16 ID) = 0;
    virtual void FreeID(u16 ID, u32 time) = 0;

    virtual CSE_Abstract* entity_Create(pcstr name) = 0;
    virtual void entity_Destroy(CSE_Abstract*& entity) = 0;

    virtual void Perform_destroy(CSE_Abstract* entity, u32 mode) = 0;
    virtual CSE_Abstract* Process_spawn(NET_Packet& packet, ClientID sender, bool mainEntityAsParent = false,
                                        CSE_Abstract* currentEntity = nullptr) = 0;

private:
#ifdef DEBUG
    bool sender_functor_invoked;
#endif
};
