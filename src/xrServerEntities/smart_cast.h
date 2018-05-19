////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cast.h
//	Created 	: 17.09.2004
//  Modified 	: 17.09.2004
//	Author		: Dmitriy Iassenev
//	Description : Smart dynamic cast
////////////////////////////////////////////////////////////////////////////

// WARNING: Due to conditional include of different implementations, this file CAN NOT have a
// #pragma once include guard. It can however have a marco-declared include guard.
#ifndef SMART_CAST_H
#define SMART_CAST_H

#ifdef DEBUG
#define PURE_DYNAMIC_CAST
#endif // DEBUG

#ifdef PURE_DYNAMIC_CAST
#define smart_cast dynamic_cast
#else

#include <typeinfo>

namespace imdex
{
using class_ptr = const uintptr_t*;
using vtable_ptr = const uintptr_t*;

static constexpr ptrdiff_t Uncached = PTRDIFF_MAX;
static constexpr ptrdiff_t Bad = PTRDIFF_MAX - 1;

template <typename T>
__forceinline vtable_ptr getVTable(const T* obj) noexcept
{
    return reinterpret_cast<vtable_ptr>(*reinterpret_cast<class_ptr>(obj));
}

template <typename To, typename From>
__forceinline To fast_dynamic_cast(From* from) noexcept
{
    thread_local static ptrdiff_t cacheOffset = Uncached;
    thread_local static vtable_ptr cacheVtable = nullptr;

    if (!from)
        return nullptr;

    using T = std::conditional_t<std::is_const_v<From>, const char, char>;

    const auto vtable = getVTable(from);
    if (vtable == cacheVtable)
    {
        switch (cacheOffset)
        {
        case Bad: return nullptr;
        case Uncached: break;
        default: return reinterpret_cast<To>(reinterpret_cast<T*>(from) + cacheOffset);
        }
    }

    cacheVtable = vtable;

    To res = dynamic_cast<To>(from);
    if (!res)
    {
        cacheOffset = Bad;
        return nullptr;
    }

    cacheOffset = reinterpret_cast<const char*>(res) - reinterpret_cast<const char*>(from);
    return res;
}

template <typename To, typename From>
__forceinline To fast_dynamic_cast(From& from)
{
    using Ptr = std::add_pointer_t<std::remove_reference_t<To>>;
    using ToPtr = std::conditional_t<std::is_const_v<To>, std::add_const_t<Ptr>, Ptr>;
    auto* res = fast_dynamic_cast<ToPtr>(std::addressof(from));
    if (!res)
#if _HAS_EXCEPTIONS
        throw std::bad_cast();
#else
        std::terminate();
#endif
    return *res;
}

} // namespace imdex

#define smart_cast imdex::fast_dynamic_cast

#endif // PURE_DYNAMIC_CAST

#endif // SMART_CAST_H
