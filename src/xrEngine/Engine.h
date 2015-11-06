#pragma once

#include "xrCore/Platform.h"

// you must define ENGINE_BUILD then building the engine itself
// and not define it if you are about to build DLL

// XXX: move all *_API macros to XRayConfig.hpp
#ifndef ENGINE_API
#ifndef NO_ENGINE_API
#ifdef ENGINE_BUILD
#define DLL_API XR_IMPORT
#define ENGINE_API XR_EXPORT
#else
#undef DLL_API
#define DLL_API XR_EXPORT
#define ENGINE_API XR_IMPORT
#endif
#else
#define ENGINE_API
#define DLL_API
#endif // !NO_ENGINE_API
#endif // !ENGINE_API

#include "engineAPI.h"
#include "eventAPI.h"
#include "xrSheduler.h"

class ENGINE_API CEngine
{
public:
    BENCH_SEC_SCRAMBLEMEMBER1
    // DLL api stuff
    CEngineAPI External;
    CEventAPI Event;
    CSheduler Sheduler;

    void Initialize();
    void Destroy();

    CEngine();
    ~CEngine();
};

ENGINE_API extern CEngine Engine;
