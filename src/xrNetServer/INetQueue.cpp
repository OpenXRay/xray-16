#include "stdafx.h"
#include "INetQueue.h"

static u32 LastTimeCreate = 0;

INetQueue::INetQueue()
#ifdef CONFIG_PROFILE_LOCKS
    : cs(MUTEX_PROFILE_ID(INetQueue))
#endif
{
    unused.reserve(128);
    for (int i = 0; i < 16; i++)
        unused.push_back(xr_new<NET_Packet>());
}

INetQueue::~INetQueue()
{
    cs.Enter();
    u32 it;
    for (it = 0; it < unused.size(); it++)
        xr_delete(unused[it]);
    for (it = 0; it < ready.size(); it++)
        xr_delete(ready[it]);
    cs.Leave();
}

NET_Packet* INetQueue::Create()
{
    NET_Packet* P = nullptr;
    //cs.Enter();
//#ifdef _DEBUG
    //Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
//#endif
    if (unused.empty())
    {
        ready.push_back(xr_new<NET_Packet>());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = CPU::GetTicks();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    //cs.Leave();
    return P;
}

NET_Packet* INetQueue::Create(const NET_Packet& _other)
{
    NET_Packet* P = nullptr;
    cs.Enter();
    //#ifdef _DEBUG
        //Msg("- INetQueue::Create - ready %d, unused %d", ready.size(), unused.size());
    //#endif
    if (unused.empty())
    {
        ready.push_back(xr_new<NET_Packet>());
        P = ready.back();
        //---------------------------------------------
        LastTimeCreate = CPU::GetTicks();
        //---------------------------------------------
    }
    else
    {
        ready.push_back(unused.back());
        unused.pop_back();
        P = ready.back();
    }
    CopyMemory(P, &_other, sizeof(NET_Packet));
    cs.Leave();
    return P;
}

NET_Packet* INetQueue::Retreive()
{
    NET_Packet* P = nullptr;
    //cs.Enter();
//#ifdef _DEBUG
    //Msg("INetQueue::Retreive - ready %d, unused %d", ready.size(), unused.size());
//#endif
    if (!ready.empty())
        P = ready.front();
    //---------------------------------------------
    else
    {
        u32 tmp_time = CPU::GetTicks() - 60000;
        u32 size = unused.size();
        if ((LastTimeCreate < tmp_time) && (size > 32))
        {
            xr_delete(unused.back());
            unused.pop_back();
        }
    }
    //---------------------------------------------
    //cs.Leave();
    return P;
}

void INetQueue::Release()
{
    //cs.Enter();
//#ifdef _DEBUG
    //Msg("INetQueue::Release - ready %d, unused %d", ready.size(), unused.size());
//#endif
    VERIFY(!ready.empty());
    //---------------------------------------------
    u32 tmp_time = CPU::GetTicks() - 60000;
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
    //cs.Leave();
}
