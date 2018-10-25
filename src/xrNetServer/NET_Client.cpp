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
#ifdef CONFIG_PROFILE_LOCKS
IPureClient::IPureClient(CTimer* timer)
    : net_Statistic(timer), net_csEnumeration(new Lock(MUTEX_PROFILE_ID(IPureClient::net_csEnumeration)))
#else
IPureClient::IPureClient(CTimer* timer) : net_Statistic(timer), net_csEnumeration(new Lock)
#endif
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
    delete net_csEnumeration;
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

    // Clean up Host _list_
    net_csEnumeration->Enter();
    for (u32 i = 0; i < net_Hosts.size(); i++)
    {
        HOST_NODE& N = net_Hosts[i];
        _RELEASE(N.pHostAddress);
    }
    net_Hosts.clear();
    net_csEnumeration->Leave();

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

void IPureClient::timeServer_Correct(u32 sv_time, u32 cl_time)
{
    u32 ping = net_Statistic.getPing();
    u32 delta = sv_time + ping / 2 - cl_time;
    net_DeltaArray.push(delta);
    Sync_Average();
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

void IPureClient::UpdateStatistic()
{
    // Query network statistic for this client
    DPN_CONNECTION_INFO CI;
    ZeroMemory(&CI, sizeof(CI));
    CI.dwSize = sizeof(CI);
    HRESULT hr = NET->GetConnectionInfo(&CI, 0);
    if (FAILED(hr))
        return;

    net_Statistic.Update(CI);
}

void IPureClient::Sync_Average()
{
    //***** Analyze results
    s64 summary_delta = 0;
    s32 size = net_DeltaArray.size();
    u32* I = net_DeltaArray.begin();
    u32* E = I + size;
    for (; I != E; I++)
        summary_delta += *((int*)I);

    s64 frac = s64(summary_delta) % s64(size);
    if (frac < 0)
        frac = -frac;
    summary_delta /= s64(size);
    if (frac > s64(size / 2))
        summary_delta += (summary_delta < 0) ? -1 : 1;
    net_TimeDelta_Calculated = s32(summary_delta);
    net_TimeDelta = (net_TimeDelta * 5 + net_TimeDelta_Calculated) / 6;
    //	Msg("* CLIENT: d(%d), dc(%d), s(%d)",net_TimeDelta,net_TimeDelta_Calculated,size);
}

bool IPureClient::net_isDisconnected() const { return net_Disconnected; }

bool IPureClient::net_IsSyncronised() { return net_Syncronised; }

IPureClient::HOST_NODE::HOST_NODE() : pdpAppDesc(new DPN_APPLICATION_DESC), pHostAddress(nullptr) {}

IPureClient::HOST_NODE::HOST_NODE(const HOST_NODE& rhs) : pdpAppDesc(new DPN_APPLICATION_DESC)
{
    *pdpAppDesc = *rhs.pdpAppDesc;
    pHostAddress = rhs.pHostAddress;
    dpSessionName = rhs.dpSessionName;
}

IPureClient::HOST_NODE::HOST_NODE(HOST_NODE&& rhs) noexcept : pdpAppDesc(rhs.pdpAppDesc)
{
    pHostAddress = rhs.pHostAddress;
    dpSessionName.swap(rhs.dpSessionName);
    rhs.pdpAppDesc = nullptr;
    rhs.pHostAddress = nullptr;
}

IPureClient::HOST_NODE::~HOST_NODE() noexcept { delete pdpAppDesc; }
