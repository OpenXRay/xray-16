#ifndef xrCoreH
#define xrCoreH
#pragma once

#ifndef DEBUG
# define MASTER_GOLD
#endif // DEBUG

//#define BENCHMARK_BUILD

#ifdef BENCHMARK_BUILD
# define BENCH_SEC_CALLCONV __stdcall
# define BENCH_SEC_SCRAMBLEVTBL1 virtual int GetFlags() { return 1;}
# define BENCH_SEC_SCRAMBLEVTBL2 virtual void* GetData() { return 0;}
# define BENCH_SEC_SCRAMBLEVTBL3 virtual void* GetCache(){ return 0;}
# define BENCH_SEC_SIGN , void *pBenchScrampleVoid = 0
# define BENCH_SEC_SCRAMBLEMEMBER1 float m_fSrambleMember1;
# define BENCH_SEC_SCRAMBLEMEMBER2 float m_fSrambleMember2;
#else // BENCHMARK_BUILD
# define BENCH_SEC_CALLCONV
# define BENCH_SEC_SCRAMBLEVTBL1
# define BENCH_SEC_SCRAMBLEVTBL2
# define BENCH_SEC_SCRAMBLEVTBL3
# define BENCH_SEC_SIGN
# define BENCH_SEC_SCRAMBLEMEMBER1
# define BENCH_SEC_SCRAMBLEMEMBER2
#endif // BENCHMARK_BUILD

#pragma warning(disable:4996)

#if (defined(_DEBUG) || defined(MIXED) || defined(DEBUG)) && !defined(FORCE_NO_EXCEPTIONS)
// "debug" or "mixed"
#if !defined(_CPPUNWIND)
#error Please enable exceptions...
#endif
#define _HAS_EXCEPTIONS 1 // STL
#define XRAY_EXCEPTIONS 1 // XRAY
#else
// "release"
#if defined(_CPPUNWIND) && !defined __BORLANDC__
#error Please disable exceptions...
#endif
#define _HAS_EXCEPTIONS 1 // STL
#define XRAY_EXCEPTIONS 0 // XRAY
#define LUABIND_NO_EXCEPTIONS
#pragma warning(disable:4530)
#endif

#if !defined(_MT)
// multithreading disabled
#error Please enable multi-threaded library...
#endif

# include "xrCore_platform.h"

/*
// stl-config
// *** disable exceptions for both STLport and VC7.1 STL
// #define _STLP_NO_EXCEPTIONS 1
// #if XRAY_EXCEPTIONS
#define _HAS_EXCEPTIONS 1 // force STL again
// #endif
*/

// *** try to minimize code bloat of STLport
#ifdef __BORLANDC__
#else
#ifdef XRCORE_EXPORTS // no exceptions, export allocator and common stuff
#define _STLP_DESIGNATED_DLL 1
#define _STLP_USE_DECLSPEC 1
#else
#define _STLP_USE_DECLSPEC 1 // no exceptions, import allocator and common stuff
#endif
#endif

// #include <exception>
// using std::exception;

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <typeinfo.h>
//#include <process.h>

#ifndef DEBUG
#ifdef _DEBUG
#define DEBUG
#endif
#ifdef MIXED
#define DEBUG
#endif
#endif

#ifdef XRCORE_STATIC
# define NO_FS_SCAN
#endif

#ifdef _EDITOR
# define NO_FS_SCAN
#endif

// inline control - redefine to use compiler's heuristics ONLY
// it seems "IC" is misused in many places which cause code-bloat
// ...and VC7.1 really don't miss opportunities for inline :)
#ifdef _EDITOR
# define __forceinline inline
#endif
#define _inline inline
#define __inline inline
#define IC inline
#define ICF __forceinline // !!! this should be used only in critical places found by PROFILER
#ifdef _EDITOR
# define ICN
#else
# define ICN __declspec (noinline)
#endif

#ifndef DEBUG
#pragma inline_depth ( 254 )
#pragma inline_recursion( on )
#ifndef __BORLANDC__
#pragma intrinsic (abs, fabs, fmod, sin, cos, tan, asin, acos, atan, sqrt, exp, log, log10, strcat)
#endif
#endif

#include <time.h>
// work-around dumb borland compiler
#ifdef __BORLANDC__
#define ALIGN(a)

#include <assert.h>
#include <utime.h>
#define _utimbuf utimbuf
#define MODULE_NAME "xrCoreB.dll"

// function redefinition
#define fabsf(a) fabs(a)
#define sinf(a) sin(a)
#define asinf(a) asin(a)
#define cosf(a) cos(a)
#define acosf(a) acos(a)
#define tanf(a) tan(a)
#define atanf(a) atan(a)
#define sqrtf(a) sqrt(a)
#define expf(a) ::exp(a)
#define floorf floor
#define atan2f atan2
#define logf log
// float redefine
#define _PC_24 PC_24
#define _PC_53 PC_53
#define _PC_64 PC_64
#define _RC_CHOP RC_CHOP
#define _RC_NEAR RC_NEAR
#define _MCW_EM MCW_EM
#else
#define ALIGN(a) __declspec(align(a))
#include <sys\utime.h>
#define MODULE_NAME "xrCore.dll"
#endif


// Warnings
#pragma warning (disable : 4251 ) // object needs DLL interface
#pragma warning (disable : 4201 ) // nonstandard extension used : nameless struct/union
#pragma warning (disable : 4100 ) // unreferenced formal parameter
#pragma warning (disable : 4127 ) // conditional expression is constant
//#pragma warning (disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled
#pragma warning (disable : 4345 )
#pragma warning (disable : 4714 ) // __forceinline not inlined
#ifndef DEBUG
#pragma warning (disable : 4189 ) // local variable is initialized but not refenced
#endif // frequently in release code due to large amount of VERIFY


#ifdef _M_AMD64
#pragma warning (disable : 4512 )
#endif

// stl
#pragma warning (push)
#pragma warning (disable:4702)
#include <algorithm>
#include <limits>
#include <vector>
#include <stack>
#include <list>
#include <set>
#include <map>

#ifndef _EDITOR
# include <hash_map>
# include <hash_set>
#endif

#include <string>
#pragma warning (pop)
#pragma warning (disable : 4100 ) // unreferenced formal parameter

// Our headers
#ifdef XRCORE_STATIC
# define XRCORE_API
#else
# ifdef XRCORE_EXPORTS
# define XRCORE_API __declspec(dllexport)
# else
# define XRCORE_API __declspec(dllimport)
# endif
#endif

#include "xrDebug.h"
#include "vector.h"

#include "clsid.h"
#include "xrSyncronize.h"
#include "xrMemory.h"
#include "xrDebug.h"

#include "_stl_extensions.h"
#include "xrsharedmem.h"
#include "xrstring.h"
#include "xr_resource.h"
#include "rt_compressor.h"
#include "xr_shared.h"
#include "string_concatenations.h"

// stl ext
struct XRCORE_API xr_rtoken
{
    shared_str name;
    int id;
    xr_rtoken(LPCSTR _nm, int _id) { name = _nm; id = _id; }
public:
    void rename(LPCSTR _nm) { name = _nm; }
    bool equal(LPCSTR _nm) { return (0 == xr_strcmp(*name, _nm)); }
};

#pragma pack (push,1)
struct XRCORE_API xr_shortcut
{
    enum
    {
        flShift = 0x20,
        flCtrl = 0x40,
        flAlt = 0x80,
    };
    union
    {
        struct
        {
            u8 key;
            Flags8 ext;
        };
        u16 hotkey;
    };
    xr_shortcut(u8 k, BOOL a, BOOL c, BOOL s) :key(k) { ext.assign(u8((a ? flAlt : 0) | (c ? flCtrl : 0) | (s ? flShift : 0))); }
    xr_shortcut() { ext.zero(); key = 0; }
    bool similar(const xr_shortcut& v)const { return ext.equal(v.ext) && (key == v.key); }
};
#pragma pack (pop)

DEFINE_VECTOR(shared_str, RStringVec, RStringVecIt);
DEFINE_SET(shared_str, RStringSet, RStringSetIt);
DEFINE_VECTOR(xr_rtoken, RTokenVec, RTokenVecIt);

#define xr_pure_interface __interface

#include "FS.h"
#include "log.h"
#include "xr_trims.h"
#include "xr_ini.h"
#ifdef NO_FS_SCAN
# include "ELocatorAPI.h"
#else
# include "LocatorAPI.h"
#endif
#include "FileSystem.h"
#include "FTimer.h"
#include "fastdelegate.h"
#include "intrusive_ptr.h"

#include "net_utils.h"

// destructor
template <class T>
class destructor
{
    T* ptr;
public:
    destructor(T* p) { ptr = p; }
    ~destructor() { xr_delete(ptr); }
    IC T& operator() ()
    {
        return *ptr;
    }
};

// ********************************************** The Core definition
class XRCORE_API xrCore
{
public:
    string64 ApplicationName;
    string_path ApplicationPath;
    string_path WorkingPath;
    string64 UserName;
    string64 CompName;
    char* Params;
    DWORD dwFrame;

public:
    void _initialize(LPCSTR ApplicationName, LogCallback cb = 0, BOOL init_fs = TRUE, LPCSTR fs_fname = 0);
    void _destroy();
};

//Borland class dll interface
#define _BCL __stdcall

//Borland global function dll interface
#define _BGCL __stdcall


extern XRCORE_API xrCore Core;

#endif

