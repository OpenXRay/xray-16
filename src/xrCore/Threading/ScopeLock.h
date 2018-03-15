#pragma once

#include "Common/Noncopyable.hpp"
#include "Threading/Lock.hpp"
#include "xrDebug.h"

class ScopeLock: Noncopyable
{
public:
    ScopeLock(Lock* SyncObject);
    ~ScopeLock();

private:
    Lock * m_SyncObject;
};

ScopeLock::ScopeLock(Lock* SyncObject): m_SyncObject(SyncObject)
{
    VERIFY(m_SyncObject);

    m_SyncObject->Enter();
}

ScopeLock::~ScopeLock()
{
    m_SyncObject->Leave();
}
