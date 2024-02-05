#ifndef __CRITICAL_SECTION_H__
#define __CRITICAL_SECTION_H__

class CryCriticalSectionNonRecursive
{
    struct CRY_SRWLOCK // From winnt.h
    {
        CRY_SRWLOCK() : SRWLock_(0)
        {
            static_assert(sizeof(SRWLock_) == sizeof(PSRWLOCK), "RWLock-pointer has invalid size");
            InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&SRWLock_));
        }
        void* SRWLock_;
    };

public:
    CryCriticalSectionNonRecursive() = default;

    void Lock() { AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)); }
    void Unlock() { ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)); }
    bool TryLock()
    {
        return TryAcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)) == TRUE;
    }

    void LockShared() { AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)); }
    void UnlockShared() { ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)); }
    bool TryLockShared()
    {
        return TryAcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_win32_lock_type.SRWLock_)) == TRUE;
    }

private:
    CryCriticalSectionNonRecursive(const CryCriticalSectionNonRecursive&) = delete;
    CryCriticalSectionNonRecursive& operator=(const CryCriticalSectionNonRecursive&) = delete;

private:
    CRY_SRWLOCK m_win32_lock_type;
};

class CryCriticalSection
{
public:
    CryCriticalSection() : m_recurseCounter(0), m_exclusiveOwningThreadId(0) {}

    void Lock()
    {
        const unsigned int threadId = ::GetCurrentThreadId();

        if (threadId == m_exclusiveOwningThreadId)
        {
            ++m_recurseCounter;
        }
        else
        {
            m_win32_lock_type.Lock();
            R_ASSERT(m_recurseCounter == 0);
            R_ASSERT(m_exclusiveOwningThreadId == 0);
            m_exclusiveOwningThreadId = threadId;
        }
    }
    void Unlock()
    {
        const unsigned threadId = ::GetCurrentThreadId();
        R_ASSERT(m_exclusiveOwningThreadId == threadId);

        if (m_recurseCounter)
        {
            --m_recurseCounter;
        }
        else
        {
            m_exclusiveOwningThreadId = 0;
            m_win32_lock_type.Unlock();
        }
    }

    bool TryLock()
    {
        const unsigned int threadId = ::GetCurrentThreadId();
        if (m_exclusiveOwningThreadId == threadId)
        {
            ++m_recurseCounter;
            return true;
        }
        else
        {
            const bool ret = (m_win32_lock_type.TryLock() == TRUE);
            if (ret)
            {
                m_exclusiveOwningThreadId = threadId;
            }
            return ret;
        }
    }

    // Deprecated
#ifndef _RELEASE
    bool IsLocked() { return m_exclusiveOwningThreadId == ::GetCurrentThreadId(); }
#endif

private:
    CryCriticalSection(const CryCriticalSection&) = delete;
    CryCriticalSection& operator=(const CryCriticalSection&) = delete;

private:
    CryCriticalSectionNonRecursive m_win32_lock_type;
    uint32_t m_recurseCounter;

    // Due to its semantics, this member can be accessed in an unprotected manner,
    // but only for comparison with the current tid.
    unsigned int m_exclusiveOwningThreadId;
};

//////////////////////////////////////////////////////////////////////////
//! CryAutoCriticalSection implements a helper class to automatically.
//! lock critical section in constructor and release on destructor.
template <class LockClass>
class CryAutoLock
{
public:
    CryAutoLock() = delete;
    CryAutoLock(const CryAutoLock<LockClass>&) = delete;
    CryAutoLock<LockClass>& operator=(const CryAutoLock<LockClass>&) = delete;

    CryAutoLock(LockClass& Lock) : m_pLock(&Lock) { m_pLock->Lock(); }
    CryAutoLock(const LockClass& Lock) : m_pLock(const_cast<LockClass*>(&Lock)) { m_pLock->Lock(); }
    ~CryAutoLock() { m_pLock->Unlock(); }

private:
    LockClass* m_pLock;
};

#endif 
