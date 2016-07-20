#pragma once
#ifndef __FLAGS_H__
#define __FLAGS_H__
#include "_types.h"

template <class T>
struct _flags
{
public:
    typedef T TYPE;
    typedef _flags<T> Self;
    typedef Self& SelfRef;
    typedef const Self& SelfCRef;

public:
    T flags;

    TYPE get() const throw() { return flags; }
    SelfRef zero() throw()
    {
        flags = T(0);
        return *this;
    }
    SelfRef one() throw()
    {
        flags = T(-1);
        return *this;
    }
    SelfRef invert() throw()
    {
        flags = ~flags;
        return *this;
    }
    SelfRef invert(const Self& f) throw()
    {
        flags = ~f.flags;
        return *this;
    }
    SelfRef invert(const T mask) throw()
    {
        flags ^= mask;
        return *this;
    }
    SelfRef assign(const Self& f) throw()
    {
        flags = f.flags;
        return *this;
    }
    SelfRef assign(const T mask) throw()
    {
        flags = mask;
        return *this;
    }
    SelfRef set(const T mask, BOOL value) throw()
    {
        if (value)
            flags |= mask;
        else
            flags &= ~mask;
        return *this;
    }
    BOOL is(const T mask) const throw() { return mask == (flags & mask); }
    BOOL is_any(const T mask) const throw() { return BOOL(!!(flags & mask)); }
    BOOL test(const T mask) const throw() { return BOOL(!!(flags & mask)); }
    SelfRef or (const T mask) throw()
    {
        flags |= mask;
        return *this;
    }
    SelfRef or (const Self& f, const T mask) throw()
    {
        flags = f.flags | mask;
        return *this;
    }
    SelfRef and (const T mask) throw()
    {
        flags &= mask;
        return *this;
    }
    SelfRef and (const Self& f, const T mask) throw()
    {
        flags = f.flags & mask;
        return *this;
    }
    BOOL equal(const Self& f) const throw() { return flags == f.flags; }
    BOOL equal(const Self& f, const T mask) const throw() { return (flags & mask) == (f.flags & mask); }
};

typedef _flags<u8> Flags8;
typedef _flags<u8> flags8;
typedef _flags<u16> Flags16;
typedef _flags<u16> flags16;
typedef _flags<u32> Flags32;
typedef _flags<u32> flags32;
typedef _flags<u64> Flags64;
typedef _flags<u64> flags64;

#endif //__FLAGS_H__
