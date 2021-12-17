#pragma once
#ifndef __FLAGS_H__
#define __FLAGS_H__
#include "xr_types.h"

template <class T>
struct _flags
{
    using TYPE = T;
    using Self =_flags<T>;
    using SelfRef = Self&;
    using SelfCRef = const Self&;

    T flags;

    TYPE get() const noexcept { return flags; }

    SelfRef zero() noexcept
    {
        flags = T(0);
        return *this;
    }

    SelfRef one() noexcept
    {
        flags = T(-1);
        return *this;
    }

    SelfRef invert() noexcept
    {
        flags = ~flags;
        return *this;
    }

    SelfRef invert(const Self& f) noexcept
    {
        flags = ~f.flags;
        return *this;
    }

    SelfRef invert(const T mask) noexcept
    {
        flags ^= mask;
        return *this;
    }

    SelfRef assign(const Self& f) noexcept
    {
        flags = f.flags;
        return *this;
    }

    SelfRef assign(const T mask) noexcept
    {
        flags = mask;
        return *this;
    }

    SelfRef set(const T mask, bool value) noexcept
    {
        if (value)
            flags |= mask;
        else
            flags &= ~mask;
        return *this;
    }

    bool is(const T mask) const noexcept { return mask == (flags & mask); }
    bool is_any(const T mask) const noexcept { return !!(flags & mask); }
    bool test(const T mask) const noexcept { return !!(flags & mask); }

    SelfRef _or(const T mask) noexcept
    {
        flags |= mask;
        return *this;
    }

    SelfRef _or(const Self& f, const T mask) noexcept
    {
        flags = f.flags | mask;
        return *this;
    }

    SelfRef _and(const T mask) noexcept
    {
        flags &= mask;
        return *this;
    }

    SelfRef _and(const Self& f, const T mask) noexcept
    {
        flags = f.flags & mask;
        return *this;
    }

    bool equal(const Self& f) const noexcept { return flags == f.flags; }
    bool equal(const Self& f, const T mask) const noexcept { return (flags & mask) == (f.flags & mask); }
};

using Flags8 = _flags<u8>;
using flags8 = _flags<u8>;
using Flags16 = _flags<u16>;
using flags16 = _flags<u16>;
using Flags32 = _flags<u32>;
using flags32 = _flags<u32>;
using Flags64 = _flags<u64>;
using flags64 = _flags<u64>;

#endif //__FLAGS_H__
