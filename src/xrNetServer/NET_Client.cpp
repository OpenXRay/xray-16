#include "stdafx.h"
#include "net_Client.h"
#include "net_Server.h"
#include "xrCore/Threading/Lock.hpp"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#include "xrCore/Debug/dxerr.h"

#ifdef CONFIG_PROFILE_LOCKS
INetQueue::INetQueue() : pcs(new Lock(MUTEX_PROFILE_ID(INetQueue)))
#else
INetQueue::INetQueue() : pcs(new Lock)
#endif
{
    unused.reserve(128);
    for (int i = 0; i < 16; i++)
        unused.push_back(new NET_Packet());
}

INetQueue::~INetQueue()
{
    pcs->Enter();
    u32 it;
    for (it = 0; it < unused.size(); it++)
        xr_delete(unused[it]);
    for (it = 0; it < ready.size(); it++)
        xr_delete(ready[it]);
    pcs->Leave();
    delete pcs;
}

static u32 LastTimeCreate = 0;

NET_Packet* INetQueue::Create()
{
    NET_Packet* P = nullptr;
    // pcs->Enter();
    //#ifdef _DEBUG
    // Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
    //#endif
    if (unused.empty())
    {
        ready.push_back(new NET_Packet());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = GetTickCount();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    // pcs->Leave();
    return P;
}

NET_Packet* INetQueue::Create(const NET_Packet& _other)
{
    NET_Packet* P = nullptr;
    pcs->Enter();
    //#ifdef _DEBUG
    // Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
    //#endif
    if (unused.empty())
    {
        ready.push_back(new NET_Packet());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = GetTickCount();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    CopyMemory(P, &_other, sizeof(NET_Packet));
    pcs->Leave();
    return P;
}

NET_Packet* INetQueue::Retreive()
{
    NET_Packet* P = nullptr;
    // pcs->Enter();
    //#ifdef _DEBUG
    // Msg("INetQueue::Retreive - ready %d, unused %d", ready.size(), unused.size());
    //#endif
    if (!ready.empty())
        P = ready.front();
    //---------------------------------------------
    else
    {
        u32 tmp_time = GetTickCount() - 60000;
        u32 size = unused.size();
        if ((LastTimeCreate < tmp_time) && (size > 32))
        {
            xr_delete(unused.back());
            unused.pop_back();
        }
    }
    //---------------------------------------------
    // pcs->Leave();
    return P;
}

void INetQueue::Release()
{
    // pcs->Enter();
    //#ifdef _DEBUG
    // Msg("INetQueue::Release - ready %d, unused %d", ready.size(), unused.size());
    //#endif
    VERIFY(!ready.empty());
    //---------------------------------------------
    u32 tmp_time = GetTickCount() - 60000;
    u32 size = unused.size();
    ready.front()->B.count = 0;
    if ((LastTimeCreate < tmp_time) && (size > 32))
    {
        xr_delete(ready.front());
    }
    else
        unused.push_back(ready.front());
    //---------------------------------------------
    ready.pop_front();
    // pcs->Leave();
}

void INetQueue::LockQ() { pcs->Enter(); }

void INetQueue::UnlockQ() { pcs->Leave(); }

//==============================================================================

IPureClient::IPureClient(CTimer* timer)
{
    device_timer = timer;
}

IPureClient::~IPureClient() {}

void IPureClient::OnMessage(void* data, u32 size)
{
    // One of the messages - decompress it
    net_Queue.LockQ();
    NET_Packet* P = net_Queue.Create();

    P->construct(data, size);
    P->timeReceive = timeServer_Async();

    u16 m_type;
    P->r_begin(m_type);
    net_Queue.UnlockQ();
}
