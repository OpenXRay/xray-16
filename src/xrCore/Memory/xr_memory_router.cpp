#include "Common/Common.hpp"
#include "xrCommon/inlining_macros.h"

#define COMPILING_XR_MEMORY_LIB

#include "xrCore/xrMemory.h"

inline void* operator new(size_t size)
{
    return Memory.mem_alloc(size);
}

inline void operator delete(void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

inline void operator delete(void* ptr, size_t) noexcept
{
    Memory.mem_free(ptr);
}
