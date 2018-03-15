#pragma once

#include "Common/Noncopyable.hpp"
#include "Threading/Lock.hpp"
#include "xrDebug.h"

class ScopeLock : Noncopyable
{
public:
    ScopeLock(Lock *SyncObject);
    ~ScopeLock();

private:
    Lock *syncObject;
};

ScopeLock::ScopeLock(Lock *SyncObject) : syncObject(SyncObject)
{
    VERIFY(syncObject);

    syncObject->Enter();
}

ScopeLock::~ScopeLock()
{
    syncObject->Leave();
}
