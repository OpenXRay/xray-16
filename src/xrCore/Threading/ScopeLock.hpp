#pragma once

#include "Common/Noncopyable.hpp"

class Lock;

class XRCORE_API ScopeLock : Noncopyable
{
    Lock* syncObject;

public:
    ScopeLock(Lock* SyncObject);
    ~ScopeLock();
};
