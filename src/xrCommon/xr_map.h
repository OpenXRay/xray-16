#pragma once
#include <map>
#include "xr_allocator.h"

template <typename K, class V, class P = std::less<K>, typename allocator = xr_allocator<std::pair<const K, V>>>
using xr_map = std::map<K, V, P, allocator>;

template <typename K, class V, class P = std::less<K>, typename allocator = xr_allocator<std::pair<const K, V>>>
using xr_multimap = std::multimap<K, V, P, allocator>;

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
