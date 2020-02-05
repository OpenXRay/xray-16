#pragma once

// Force link our lib
#ifndef COMPILING_XR_MEMORY_LIB
#pragma comment(lib, "xrMemoryLib.lib")
#endif

[[nodiscard]] void* operator new(size_t size);
[[nodiscard]] void* operator new(size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
void operator delete(void* ptr) noexcept;
void operator delete(void* ptr, std::align_val_t alignment) noexcept;
void operator delete(void* ptr, size_t) noexcept;
void operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept;
