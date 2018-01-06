#pragma once

#include "_types.h"
#include "_rect.h"
#include "_plane.h"
#include "_vector2.h"
#include "_vector3d.h"
#include "_color.h"
#include "_std_extensions.h"
#include "xrMemory.h"
#include "xrCore/Memory/XRayAllocator.hpp"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_map.h"
#include "xrCommon/xr_set.h"
#include "xrCommon/xr_stack.h"
#include "xrCommon/xr_list.h"
#include "xrCommon/xr_deque.h"
#include "xrstring.h"
#include "xrCommon/inlining_macros.h"
#include "xrCommon/xr_string.h"
#include "xrDebug_macros.h" // only for pragma todo. Remove once handled.

#pragma todo("tamlin: This header includes pretty much every std collection there are. Compiler-hog! FIX!")

using std::swap;

#ifndef _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#define _SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS
#endif
#include <hash_map>
template <typename K, class V, class _Traits = stdext::hash_compare<K, std::less<K>>,
    typename allocator = XRay::xray_allocator<std::pair<K, V>>>
class xr_hash_map : public stdext::hash_map<K, V, _Traits, allocator>
{
public:
    u32 size() const { return (u32) __super ::size(); }
};

#include "xrCommon/predicates.h"

// tamlin: TODO (low priority, for a rainy day): Rename these macros from DEFINE_* to DECLARE_*
// Xottab_DUTY: TODO: or maybe use Im-Dex variant (Get rid of this DEFINE macroses)

// STL extensions
#define DEFINE_SVECTOR(T, C, N, I) \
    typedef svector<T, C> N;       \
    typedef N::iterator I;
#include "FixedVector.h"
#include "buffer_vector.h"
#include "xr_vector_defs.h"
