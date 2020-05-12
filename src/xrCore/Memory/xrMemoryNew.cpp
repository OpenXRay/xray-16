#include "Common/Common.hpp"

#define COMPILING_XR_MEMORY_LIB

#include "xrCore/xrMemory.h"

[[nodiscard]] void* operator new(size_t size)
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&)
{
    return Memory.mem_alloc(size);
}

[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment)
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&)
{
    return Memory.mem_alloc(size, static_cast<size_t>(alignment));
}

void operator delete(void* ptr) noexcept
{
    Memory.mem_free(ptr);
}

void  operator delete(void* ptr, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}

void operator delete(void* ptr, size_t) noexcept
{
    Memory.mem_free(ptr);
}

void  operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept
{
    Memory.mem_free(ptr, static_cast<size_t>(alignment));
}
