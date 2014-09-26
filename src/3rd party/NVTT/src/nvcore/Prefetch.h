// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_PREFETCH_H
#define NV_CORE_PREFETCH_H

#include <nvcore/nvcore.h>

// nvPrefetch
#if NV_CC_GNUC

#define nvPrefetch(ptr)	__builtin_prefetch(ptr)

#elif NV_CC_MSVC 

#if NV_CPU_X86
__forceinline void nvPrefetch(const void * mem)
{
	__asm mov ecx, mem
	__asm prefetcht0 [ecx];
//	__asm prefetchnta [ecx];
}
#endif // NV_CPU_X86

#else // NV_CC_MSVC

// do nothing in other case.
#define nvPrefetch(ptr)

#endif // NV_CC_MSVC

#endif // NV_CORE_PREFETCH_H
