#pragma once
#include <map>
#include "xalloc.h"
#include "xrCore/xrDebug_macros.h"

template <typename K, class V, class P = std::less<K>, typename allocator = xalloc<std::pair<K, V>>>
class xr_map : public std::map<K, V, P, allocator>
{
public:
    u32 size() const { return (u32)std::map<K, V, P, allocator>::size(); }
};

template <typename K, class V, class P = std::less<K>, typename allocator = xalloc<std::pair<K, V>>>
class xr_multimap : public std::multimap<K, V, P, allocator>
{
public:
    u32 size() const { return (u32)std::multimap<K, V, P, allocator>::size(); }
};

#define DEF_MAP(N, K, T)\
    typedef xr_map<K, T> N;\
    typedef N::iterator N##_it;

#define DEFINE_MAP(K, T, N, I)\
    typedef xr_map<K, T> N;\
    typedef N::iterator I;

#define DEFINE_MAP_PRED(K, T, N, I, P)\
    typedef xr_map<K, T, P> N;\
    typedef N::iterator I;

#define DEFINE_MMAP(K, T, N, I)\
    typedef xr_multimap<K, T> N;\
    typedef N::iterator I;
