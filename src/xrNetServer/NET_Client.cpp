#include "stdafx.h"
#include "net_Client.h"
#include "net_Server.h"
#include "net_Messages.h"
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
} net_DeltaArray;

//-------
XRNETSERVER_API Flags32 psNET_Flags = {0};
XRNETSERVER_API int psNET_ClientUpdate = 30; // FPS
XRNETSERVER_API int psNET_ClientPending = 2;
XRNETSERVER_API char psNET_Name[32] = "Player";
XRNETSERVER_API bool psNET_direct_connect = false;

//==============================================================================

IPureClient::IPureClient(CTimer* timer) : net_Statistic(timer)
{
    NET = nullptr;
    device_timer = timer;
    net_TimeDelta_User = 0;
    net_Time_LastUpdate = 0;
    net_TimeDelta = 0;
    net_TimeDelta_Calculated = 0;
}

IPureClient::~IPureClient()
{
    psNET_direct_connect = false;
}

bool IPureClient::Connect(pcstr options)
{
    R_ASSERT(options);
    net_Disconnected = false;

    // Sync
    net_TimeDelta = 0;
    return true;
}

void IPureClient::Disconnect()
{
    if (NET)
        NET->Close(0);

    // Release interfaces
    _SHOW_REF("cl_netCORE", NET);
    _RELEASE(NET);

    net_Connected = EnmConnectionWait;
    net_Syncronised = false;
}

void IPureClient::OnMessage(void* data, u32 size)
{
    // One of the messages - decompress it
    net_Queue.LockQ();
    NET_Packet* P = net_Queue.Create();

    P->construct(data, size);
    P->timeReceive = timeServer_Async(); // TimerAsync(device_timer);

    u16 m_type;
    P->r_begin(m_type);
    net_Queue.UnlockQ();
}

void IPureClient::SendTo_LL(void* data, u32 size, u32 dwFlags, u32 dwTimeout)
{
    if (net_Disconnected)
        return;

    DPN_BUFFER_DESC desc;

    desc.dwBufferSize = size;
    desc.pBufferData = (BYTE*)data;

    net_Statistic.dwBytesSended += size;

    // verify
    VERIFY(desc.dwBufferSize);
    VERIFY(desc.pBufferData);
    VERIFY(NET);

    DPNHANDLE hAsync = 0;
    HRESULT hr = NET->Send(&desc, 1, dwTimeout, nullptr, &hAsync, dwFlags | DPNSEND_COALESCE);

    //	Msg("- Client::SendTo_LL [%d]", size);
    if (FAILED(hr))
    {
        Msg("! ERROR: Failed to send net-packet, reason: %s", xrDebug::ErrorToString(hr));
        // pcstr x = DXGetErrorString9(hr);
        string1024 tmp = "";
        DXTRACE_ERR(tmp, hr);
    }
}

bool IPureClient::net_IsSyncronised() { return net_Syncronised; }
