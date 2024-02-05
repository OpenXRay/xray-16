/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SMARTPTR_H
#define CRYINCLUDE_CRYCOMMON_SMARTPTR_H
#pragma once

#include <type_traits>

#if defined(APPLE)
#include <cstddef>
#endif

//////////////////////////////////////////////////////////////////
// SMART POINTER
//////////////////////////////////////////////////////////////////
template <class _I>
class _smart_ptr
{
private:
    _I* p;

public:
    _smart_ptr() : p(NULL) {}
    _smart_ptr(_I* p_)
    {
        p = p_;
        if (p)
        {
            p->AddRef();
        }
    }

    _smart_ptr(const _smart_ptr& p_)
    {
        p = p_.p;
        if (p)
        {
            p->AddRef();
        }
    }

    template <typename _Y>
    _smart_ptr(const _smart_ptr<_Y>& p_)
    {
        p = p_.get();
        if (p)
        {
            p->AddRef();
        }
    }

    ~_smart_ptr()
    {
        if (p)
        {
            p->Release();
        }
    }
    operator _I*() const { return p; }

    _I& operator*() const { return *p; }
    _I* operator->(void) const { return p; }
    _I* get() const { return p; }
    _smart_ptr& operator=(_I* newp)
    {
        if (newp)
        {
            newp->AddRef();
        }
        if (p)
        {
            p->Release();
        }
        p = newp;
        return *this;
    }

    void reset() { _smart_ptr<_I>().swap(*this); }

    void reset(_I* ptr) { _smart_ptr<_I>(ptr).swap(*this); }

    void attach(_I* p_) { p = p_; }

    _smart_ptr& operator=(const _smart_ptr& newp)
    {
        if (newp.p)
        {
            newp.p->AddRef();
        }
        if (p)
        {
            p->Release();
        }
        p = newp.p;
        return *this;
    }

    template <typename _Y>
    _smart_ptr& operator=(const _smart_ptr<_Y>& newp)
    {
        _I* const p2 = newp.get();
        if (p2)
        {
            p2->AddRef();
        }
        if (p)
        {
            p->Release();
        }
        p = p2;
        return *this;
    }
   
    void swap(_smart_ptr<_I>& other) { std::swap(p, other.p); }
};

template <typename T>
IC void swap(_smart_ptr<T>& a, _smart_ptr<T>& b)
{
    a.swap(b);
}

// reference target without vtable for smart pointer
// implements AddRef() and Release() strategy using reference counter of the specified type
template <typename TDerived, typename Counter = int>
class _reference_target_no_vtable
{
public:
    _reference_target_no_vtable() : m_nRefCounter(0) {}

    ~_reference_target_no_vtable()
    {
        // assert (!m_nRefCounter);
    }

    void AddRef()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter >= 0);
        ++m_nRefCounter;
    }

    void Release()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter > 0);
        if (--m_nRefCounter == 0)
        {
            delete static_cast<TDerived*>(this);
        }
        else if (m_nRefCounter < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
    }
    // Warning: use for debugging/statistics purposes only!
    Counter NumRefs() { return m_nRefCounter; }

protected:
    Counter m_nRefCounter;
};

// reference target with vtable for smart pointer
// implements AddRef() and Release() strategy using reference counter of the specified type
template <typename Counter>
class _reference_target
{
public:
    _reference_target() : m_nRefCounter(0) {}

    virtual ~_reference_target()
    {
        // assert (!m_nRefCounter);
    }

    void AddRef()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter >= 0);
        ++m_nRefCounter;
    }

    void Release()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter > 0);
        if (--m_nRefCounter == 0)
        {
            delete this;
        }
        else if (m_nRefCounter < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
    }
    // Warning: use for debugging/statistics purposes only!
    Counter NumRefs() { return m_nRefCounter; }

protected:
    Counter m_nRefCounter;
};

// default implementation is int counter - for better alignment
typedef _reference_target<int> _reference_target_t;

// reference target for smart pointer with configurable destruct function
// implements AddRef() and Release() strategy using reference counter of the specified type
template <typename T, typename Counter = int>
class _cfg_reference_target
{
public:
    typedef void (*DeleteFncPtr)(void*);

    _cfg_reference_target() : m_nRefCounter(0), m_pDeleteFnc(operator delete) {}

    explicit _cfg_reference_target(DeleteFncPtr pDeleteFnc) : m_nRefCounter(0), m_pDeleteFnc(pDeleteFnc) {}

    virtual ~_cfg_reference_target() {}

    void AddRef()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter >= 0);
        ++m_nRefCounter;
    }

    void Release()
    {
        CHECK_REFCOUNT_CRASH(m_nRefCounter > 0);
        if (--m_nRefCounter == 0)
        {
            assert(m_pDeleteFnc);
            static_cast<T*>(this)->~T();
            m_pDeleteFnc(this);
        }
        else if (m_nRefCounter < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
    }

    // Sets the delete function with which this object is supposed to be deleted
    void SetDeleteFnc(DeleteFncPtr pDeleteFnc) { m_pDeleteFnc = pDeleteFnc; }

    // Warning: use for debugging/statistics purposes only!
    Counter NumRefs() { return m_nRefCounter; }

protected:
    Counter m_nRefCounter;
    DeleteFncPtr m_pDeleteFnc;
};

// base class for interfaces implementing reference counting
// derive your interface from this class and the descendants won't have to implement
// the reference counting logic
template <typename Counter>
class _i_reference_target
{
public:
    _i_reference_target() : m_nRefCounter(0) {}

    virtual ~_i_reference_target() {}

    virtual void AddRef() { ++m_nRefCounter; }

    virtual void Release()
    {
        if (--m_nRefCounter == 0)
        {
            delete this;
        }
        else if (m_nRefCounter < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
    }

    // Warning: use for debugging/statistics purposes only!
    Counter NumRefs() const { return m_nRefCounter; }

protected:
    Counter m_nRefCounter;
};

class CMultiThreadRefCount
{
public:
    CMultiThreadRefCount() : m_cnt(0) {}
    virtual ~CMultiThreadRefCount() {}

    inline int AddRef() { return InterlockedIncrement((volatile LONG*)&m_cnt); }

    inline int Release()
    {
        const int nCount = InterlockedDecrement((volatile LONG*)&m_cnt);
        assert(nCount >= 0);
        if (nCount == 0)
        {
            delete this;
        }
        else if (nCount < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
        return nCount;
    }

    inline int GetRefCount() const { return m_cnt; }

protected:
    // Allows the memory for the object to be deallocated in the dynamic module where it was originally constructed, as
    // it may use different memory manager (Debug/Release configurations)
    virtual void DeleteThis() { delete this; }

private:
    volatile int m_cnt;
};

// base class for interfaces implementing reference counting that needs to be thread-safe
// derive your interface from this class and the descendants won't have to implement
// the reference counting logic
template <typename Counter>
class _i_multithread_reference_target
{
public:
    _i_multithread_reference_target() : m_nRefCounter(0) {}

    virtual ~_i_multithread_reference_target() {}

    virtual void AddRef() { InterlockedIncrement(&m_nRefCounter); }

    virtual void Release()
    {
        const int nCount = CryInterlockedDecrement(&m_nRefCounter);
        assert(nCount >= 0);
        if (nCount == 0)
        {
            delete this;
        }
        else if (nCount < 0)
        {
            assert(0);
            // CryFatalError("Deleting Reference Counted Object Twice");
        }
    }

    Counter NumRefs() const { return m_nRefCounter; }

protected:
    volatile Counter m_nRefCounter;
};

typedef _i_reference_target<int> _i_reference_target_t;
typedef _i_multithread_reference_target<int> _i_multithread_reference_target_t;

// TYPEDEF_AUTOPTR macro, declares Class_AutoPtr, which is the smart pointer to the given class,
// and Class_AutoArray, which is the array(STL vector) of autopointers
#ifdef ENABLE_NAIIVE_AUTOPTR
// naiive autopointer makes it easier for Visual Assist to parse the declaration and sometimes is easier for debug
#define TYPEDEF_AUTOPTR(T)                                                                                             \
    typedef T* T##_AutoPtr;                                                                                            \
    typedef std::vector<T##_AutoPtr> T##_AutoArray;
#else
#define TYPEDEF_AUTOPTR(T)                                                                                             \
    typedef _smart_ptr<T> T##_AutoPtr;                                                                                 \
    typedef std::vector<T##_AutoPtr> T##_AutoArray;
#endif

#endif // CRYINCLUDE_CRYCOMMON_SMARTPTR_H
