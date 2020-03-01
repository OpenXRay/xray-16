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

#define MACRO_TO_STRING_HELPER(a) #a
#define MACRO_TO_STRING(a) MACRO_TO_STRING_HELPER(a)

#define CONCATENIZE_HELPER(a, b) a##b
#define CONCATENIZE(a, b) CONCATENIZE_HELPER(a, b)

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
        : name(_nm), id(_id) {}

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
#include "Threading/ThreadUtil.h"

#if __has_include(".GitInfo.hpp")
#include ".GitInfo.hpp"
#endif

#ifndef GIT_INFO_CURRENT_COMMIT
#define GIT_INFO_CURRENT_COMMIT unknown
#endif

#ifndef GIT_INFO_CURRENT_BRANCH
#define GIT_INFO_CURRENT_BRANCH unknown
#endif

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
    u32 buildId; // XXX: Make constexpr
    static constexpr pcstr buildDate = __DATE__;
    static constexpr pcstr buildCommit = MACRO_TO_STRING(GIT_INFO_CURRENT_COMMIT);
    static constexpr pcstr buildBranch = MACRO_TO_STRING(GIT_INFO_CURRENT_BRANCH);

public:
    xrCore();

    string64 ApplicationName;
    string64 ApplicationTitle;
    string_path ApplicationPath;
    string_path WorkingPath;
    string64 UserName;
    string64 CompName;
    char* Params;
    DWORD dwFrame;
    bool PluginMode;

    void Initialize(
        pcstr ApplicationName, pcstr commandLine = nullptr, LogCallback cb = nullptr, bool init_fs = true, pcstr fs_fname = nullptr, bool plugin = false);
    void _destroy();

    u32 GetBuildId() const { return buildId; }
    static constexpr pcstr GetBuildDate() { return buildDate; }
    static constexpr pcstr GetBuildCommit() { return buildCommit; }
    static constexpr pcstr GetBuildBranch() { return buildBranch; }

    static constexpr pcstr GetBuildConfiguration();

    void CoInitializeMultithreaded() const;

private:
    void CalculateBuildId();
};

extern XRCORE_API xrCore Core;
