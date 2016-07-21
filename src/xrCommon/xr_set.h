#pragma once
#include <set>
#include "xalloc.h"
#include "xrCore/xrDebug_macros.h"

template <typename K, class P = std::less<K>, typename allocator = xalloc<K>>
class xr_set : public std::set<K, P, allocator>
{
public:
    u32 size() const { return (u32)std::set<K, P, allocator>::size(); }
};

template <typename K, class P = std::less<K>, typename allocator = xalloc<K>>
class xr_multiset : public std::multiset<K, P, allocator>
{
public:
    u32 size() const { return (u32)std::multiset<K, P, allocator>::size(); }
};

#define DEFINE_SET(T, N, I)\
    typedef xr_set<T> N;\
    typedef N::iterator I;

#define DEFINE_SET_PRED(T, N, I, P)\
    typedef xr_set<T, P> N;\
    typedef N::iterator I;
