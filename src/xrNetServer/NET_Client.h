#pragma once
#include "NET_Shared.h"
#include "xrCommon/xr_deque.h"
#include "xrCommon/xr_vector.h"
#include "Common/Noncopyable.hpp"
#include "xrCore/xrstring.h"

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

class XRNETSERVER_API IPureClient : Noncopyable
{
protected:
    CTimer* device_timer;

    INetQueue net_Queue;

public:
    IPureClient(CTimer* tm);
    virtual ~IPureClient();

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

    // time management
    u32 timeServer() const { return device_timer->GetElapsed_ms(); }
    u32 timeServer_Async() const {return device_timer->GetElapsed_ms(); }
};
