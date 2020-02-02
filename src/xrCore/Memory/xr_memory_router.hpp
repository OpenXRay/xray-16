#pragma once

// Force link our lib
#ifndef COMPILING_XR_MEMORY_LIB
#pragma comment(lib, "xrMemoryLib.lib")
#endif

inline void* operator new(size_t size);
inline void  operator delete(void* ptr) noexcept;
inline void  operator delete(void* ptr, size_t) noexcept;
