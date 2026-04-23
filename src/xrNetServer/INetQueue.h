#pragma once

#include "xrCommon/xr_deque.h"

class XRNETSERVER_API INetQueue : Noncopyable
{
    Lock cs;
    xr_deque<NET_Packet*> ready;
    xr_vector<NET_Packet*> unused;

public:
    INetQueue();
    ~INetQueue();

    NET_Packet* Create();
    NET_Packet* Create(const NET_Packet& _other);
    NET_Packet* Retreive();
    void Release();
    void Lock() { cs.Enter(); }
    void Unlock() { cs.Leave(); }
};
