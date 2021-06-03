#pragma once

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

#ifdef XR_ARCHITECTURE_X64
#pragma warning(disable : 4512)
#endif

#pragma warning(disable : 4714) // __forceinline not inlined

#ifndef DEBUG
#pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif // frequently in release code due to large amount of VERIFY

// Our headers
#ifdef XRAY_STATIC_BUILD
#   define XRCORE_API
#else
#   ifdef XRCORE_EXPORTS
#      define XRCORE_API XR_EXPORT
#   else
#      define XRCORE_API XR_IMPORT
#   endif
#endif

#include "xrCore_benchmark_macros.h"

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
#ifdef XR_PLATFORM_WINDOWS
#include "intrusive_ptr.h"
#endif

#include "net_utils.h"
#include "Threading/ThreadUtil.h"

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
    u32 buildId;
    static const pcstr buildDate;
    static const pcstr buildCommit;
    static const pcstr buildBranch;

public:
    xrCore();

    string64 ApplicationName;
    string64 ApplicationTitle;
    string_path ApplicationPath;
    string_path WorkingPath;
    string64 UserName;
    string64 CompName;
    char* Params;
    u32 dwFrame;
    bool PluginMode;

    void Initialize(
        pcstr ApplicationName, pcstr commandLine = nullptr, LogCallback cb = nullptr, bool init_fs = true, pcstr fs_fname = nullptr, bool plugin = false);
    void _destroy();

    u32 GetBuildId() const { return buildId; }
    static pcstr GetBuildDate() { return buildDate; }
    static pcstr GetBuildCommit() { return buildCommit; }
    static pcstr GetBuildBranch() { return buildBranch; }

    static constexpr pcstr GetBuildConfiguration();

    void CoInitializeMultithreaded() const;

private:
    void CalculateBuildId();
};

extern XRCORE_API xrCore Core;
