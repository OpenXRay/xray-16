#pragma once
// XXX: upgrade std hash structures
#ifndef _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#endif
#include <hash_map>
#include <unordered_map>
#include "xrCore/Memory/XRayAllocator.hpp"

template <typename K, class V, class _Traits = stdext::hash_compare<K, std::less<K>>, typename allocator = XRay::xray_allocator<std::pair<K, V>>>
using xr_hash_map = stdext::hash_map<K, V, _Traits, allocator>;

template<class Key, class T, class Hasher = std::hash<Key>, class KeyEqual = std::equal_to<Key>, class allocator = XRay::xray_allocator<std::pair<const Key, T>>>
using xr_unordered_map = std::unordered_map<Key, T, Hasher, KeyEqual, allocator>;
