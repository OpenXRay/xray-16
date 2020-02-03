#pragma once

// Force link our lib
#ifndef COMPILING_XR_MEMORY_LIB
#pragma comment(lib, "xrMemoryLib.lib")
#endif

[[nodiscard]] inline void* operator new(size_t size);
[[nodiscard]] inline void* operator new(size_t size, const std::nothrow_t&) noexcept;
[[nodiscard]] inline void* operator new(size_t size, std::align_val_t alignment);
[[nodiscard]] inline void* operator new(size_t size, std::align_val_t alignment, const std::nothrow_t&) noexcept;
inline void  operator delete(void* ptr) noexcept;
inline void  operator delete(void* ptr, std::align_val_t alignment) noexcept;
inline void  operator delete(void* ptr, size_t) noexcept;
inline void  operator delete(void* ptr, size_t, std::align_val_t alignment) noexcept;
