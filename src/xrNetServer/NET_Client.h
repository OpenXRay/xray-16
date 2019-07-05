#pragma once

#include "NET_Shared.h"
#include "xrCommon/xr_deque.h"
#include "xrCommon/xr_vector.h"
#include "Common/Noncopyable.hpp"
#include "xrCore/xrstring.h"

struct ip_address;

class XRNETSERVER_API INetQueue : Noncopyable
{
    Lock* pcs;
    xr_deque<NET_Packet*> ready;
    xr_vector<NET_Packet*> unused;

public:
    INetQueue();
    ~INetQueue();

    NET_Packet* Create();
    NET_Packet* Create(const NET_Packet& _other);
    NET_Packet* Retreive();
    void Release();
    void LockQ();
    void UnlockQ();
};

//==============================================================================

// DPlay
extern "C"
{
    typedef struct _DPN_APPLICATION_DESC DPN_APPLICATION_DESC;
    struct IDirectPlay8Address;
    struct IDirectPlay8Client;
}

class XRNETSERVER_API IPureClient : Noncopyable
{
    enum ConnectionState
    {
        EnmConnectionFails = 0,
        EnmConnectionWait = -1,
        EnmConnectionCompleted = 1
    };

protected:
    GameDescriptionData m_game_description;
    CTimer* device_timer;

    IDirectPlay8Client* NET;

    ConnectionState net_Connected;
    bool net_Syncronised;
    bool net_Disconnected;

    INetQueue net_Queue;
    IClientStatistic net_Statistic;

    u32 net_Time_LastUpdate;
    s32 net_TimeDelta;
    s32 net_TimeDelta_Calculated;
    s32 net_TimeDelta_User;

    void SetClientID(ClientID const& local_client) { net_ClientID = local_client; }
    IC virtual void SendTo_LL(void* data, u32 size, u32 dwFlags = 0x0008 /*DPNSEND_GUARANTEED*/, u32 dwTimeout = 0);

public:
    IPureClient(CTimer* tm);
    virtual ~IPureClient();

    bool Connect(pcstr server_name);
    void Disconnect();

    bool net_isCompleted_Connect() const { return net_Connected == EnmConnectionCompleted; }
    bool net_isFails_Connect() const { return net_Connected == EnmConnectionFails; }
    bool net_isCompleted_Sync() const { return net_Syncronised; }
    GameDescriptionData const& get_net_DescriptionData() const { return m_game_description; }
    // receive
    void StartProcessQueue() { net_Queue.LockQ(); } // WARNING ! after Start must be End !!! <-
    virtual NET_Packet* net_msg_Retreive() { return net_Queue.Retreive(); } //							|
    void net_msg_Release() { net_Queue.Release(); } //							|
    void EndProcessQueue() { net_Queue.UnlockQ(); } //							<-
    // send
    virtual void OnMessage(void* data, u32 size);
    virtual void OnInvalidHost() {}
    virtual void OnInvalidPassword() {}
    virtual void OnSessionFull() {}
    virtual void OnConnectRejected() {}
    ClientID const& GetClientID() const { return net_ClientID; }

    // time management
    u32 timeServer() const { return TimeGlobal(device_timer) + net_TimeDelta + net_TimeDelta_User; }
    u32 timeServer_Async() const { return TimerAsync(device_timer) + net_TimeDelta + net_TimeDelta_User; }
    u32 timeServer_Delta() const { return net_TimeDelta; }
    void timeServer_UserDelta(s32 d) { net_TimeDelta_User = d; }

    virtual bool net_IsSyncronised();

    virtual pcstr GetMsgId2Name(u16 ID) { return ""; }
    virtual void OnSessionTerminate(pcstr reason) {}

    virtual bool TestLoadBEClient() { return false; }

private:
    ClientID net_ClientID;
};
