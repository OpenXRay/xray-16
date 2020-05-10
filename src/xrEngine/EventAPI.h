#pragma once

#include "xrCore/Threading/Lock.hpp"
#include "Common/Noncopyable.hpp"
#include "xrCommon/xr_vector.h"

class ENGINE_API CEvent;
typedef CEvent* EVENT;

//---------------------------------------------------------------------
class ENGINE_API IEventReceiver
{
public:
    virtual void OnEvent(EVENT E, u64 P1, u64 P2) = 0;
};
//---------------------------------------------------------------------
class ENGINE_API CEventAPI : private Noncopyable
{
    struct Deferred
    {
        EVENT E;
        u64 P1;
        u64 P2;
    };

private:
    xr_vector<EVENT> Events;
    xr_vector<Deferred> Events_Deferred;
    Lock CS;

public:
#ifdef CONFIG_PROFILE_LOCKS
    CEventAPI() : CS(MUTEX_PROFILE_ID(CEventAPI)) {}
#endif // CONFIG_PROFILE_LOCKS

    EVENT Create(const char* N);
    void Destroy(EVENT& E);

    EVENT Handler_Attach(const char* N, IEventReceiver* H);
    void Handler_Detach(EVENT& E, IEventReceiver* H);

    void Signal(EVENT E, u64 P1 = 0, u64 P2 = 0);
    void Signal(pcstr E, u64 P1 = 0, u64 P2 = 0);
    void Defer(EVENT E, u64 P1 = 0, u64 P2 = 0);
    void Defer(pcstr E, u64 P1 = 0, u64 P2 = 0);

    void OnFrame();
    void Dump();
    bool Peek(pcstr EName);

    void _destroy();
};
