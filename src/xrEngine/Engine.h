#pragma once

#ifdef XRAY_STATIC_BUILD
#    define ENGINE_API
#else
#    ifdef ENGINE_BUILD
#        define ENGINE_API XR_EXPORT
#    else
#        define ENGINE_API XR_IMPORT
#    endif
#endif

#include "EngineAPI.h"
#include "EventAPI.h"
#include "xrSheduler.h"

class ENGINE_API CEngine
{
public:
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
