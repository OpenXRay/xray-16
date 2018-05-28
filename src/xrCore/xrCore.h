#pragma once

#ifndef DEBUG
#define MASTER_GOLD
#endif // DEBUG

#include "xrCore_benchmark_macros.h"

#if !defined(_CPPUNWIND)
#error Please enable exceptions...
#endif

#ifndef _MT
#error Please enable multi-threaded library...
#endif

#ifdef NDEBUG
#define XRAY_EXCEPTIONS 0
#define LUABIND_NO_EXCEPTIONS
#else
#define XRAY_EXCEPTIONS 1
#endif

#if !defined(DEBUG) && (defined(_DEBUG) || defined(MIXED))
#define DEBUG
#endif

// Warnings
#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4251) // object needs DLL interface
#pragma warning(disable : 4345)
//#pragma warning (disable : 4530 ) // C++ exception handler used, but unwind semantics are not enabled

#ifdef XR_X64
#pragma warning(disable : 4512)
#endif

#pragma warning(disable : 4714) // __forceinline not inlined

#ifndef DEBUG
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif // frequently in release code due to large amount of VERIFY

// Our headers
#include "xrDebug.h"
//#include "vector.h"

#include "clsid.h"
//#include "Threading/Lock.hpp"
#include "xrMemory.h"

//#include "_stl_extensions.h"
#include "_std_extensions.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_set.h"
#include "xrsharedmem.h"
#include "xrstring.h"
#include "xr_resource.h"
#include "Compression/rt_compressor.h"
#include "xr_shared.h"
#include "string_concatenations.h"
#include "_flags.h"

// stl ext
struct XRCORE_API xr_rtoken
{
    shared_str name;
    int id;

    xr_rtoken(pcstr _nm, int _id)
    {
        name = _nm;
        id = _id;
    }

    void rename(pcstr _nm) { name = _nm; }
    bool equal(pcstr _nm) const { return (0 == xr_strcmp(*name, _nm)); }
};

#include "xr_shortcut.h"

using RStringVec = xr_vector<shared_str>;
using RStringSet = xr_set<shared_str>;
using RTokenVec = xr_vector<xr_rtoken>;

#include "FS.h"
#include "log.h"
#include "xr_trims.h"
#include "xr_ini.h"
#ifdef NO_FS_SCAN
#include "ELocatorAPI.h"
#else
#include "LocatorAPI.h"
#endif
#include "FileSystem.h"
#include "FTimer.h"
#include "fastdelegate.h"
#ifdef WINDOWS
#include "intrusive_ptr.h"
#endif

#include "net_utils.h"

// destructor
template <class T>
class destructor
{
    T* ptr;

public:
    destructor(T* p) { ptr = p; }
    ~destructor() { xr_delete(ptr); }
    T& operator()() { return *ptr; }
};

// ***** The Core definition *****
class XRCORE_API xrCore
{
    const char* buildDate = "";
    u32 buildId = 0;

public:
    string64 ApplicationName;
    string_path ApplicationPath;
    string_path WorkingPath;
    string64 UserName;
    string64 CompName;
    char* Params;
    DWORD dwFrame;
    bool PluginMode;

    void Initialize(
        pcstr ApplicationName, LogCallback cb = nullptr, bool init_fs = true, pcstr fs_fname = nullptr, bool plugin = false);
    void _destroy();
    const char* GetBuildDate() const { return buildDate; }
    u32 GetBuildId() const { return buildId; }
    static constexpr pcstr GetBuildConfiguration();

private:
    void CalculateBuildId();
};

extern XRCORE_API xrCore Core;
