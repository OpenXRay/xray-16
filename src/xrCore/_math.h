#pragma once
#ifndef __XR_MATH_H__
#define __XR_MATH_H__

#include "_types.h"
#include "cpuid.h"
#include "xrCommon/inlining_macros.h"
#ifdef _MSC_VER
#include <intrin.h> // __rdtsc
#endif

namespace FPU
{
XRCORE_API void m24(void);
XRCORE_API void m24r(void);
XRCORE_API void m53(void);
XRCORE_API void m53r(void);
XRCORE_API void m64(void);
XRCORE_API void m64r(void);
};
namespace CPU
{
XRCORE_API extern u64 qpc_freq;
XRCORE_API extern u32 qpc_counter;

XRCORE_API extern processor_info ID;
XRCORE_API extern u64 QPC() noexcept;

#ifdef _MSC_VER
// XXX: Stale checks. All MS' x86&x64 compilers cabable of compiling XRay-16 have __rdtsc
//#ifndef _M_AMD64
// This must have been an MS compiler from the stone age. Even MSVC6 (from 1998) understands __asm rdtsc.
//#pragma warning(push)
//#pragma warning(disable : 4035)
//IC u64 GetCLK()
//{
//    _asm _emit 0x0F;
//    _asm _emit 0x31;
//}
//#pragma warning(pop)
//#else
IC u64 GetCLK() { return __rdtsc(); }
//#endif
#endif // _MSC_VER, previously M_VISUAL

#ifdef M_BORLAND
XRCORE_API u64 __fastcall GetCLK();
#endif
};

extern XRCORE_API void _initialize_cpu();
extern XRCORE_API void _initialize_cpu_thread();

// threading
typedef void thread_t(void*);
extern XRCORE_API void thread_name(const char* name);
extern XRCORE_API void thread_spawn(thread_t* entry, const char* name, unsigned stack, void* arglist);

#endif //__XR_MATH_H__
