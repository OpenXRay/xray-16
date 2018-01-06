#pragma once
#include <unordered_map>
#include "xrCore/Memory/XRayAllocator.hpp"

template <class Key, class T, class Hasher = std::hash<Key>, class KeyEqual = std::equal_to<Key>,
          class allocator = XRay::xray_allocator<std::pair<const Key, T>>>
using xr_unordered_map = std::unordered_map<Key, T, Hasher, KeyEqual, allocator>;
