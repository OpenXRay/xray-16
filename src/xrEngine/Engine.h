#pragma once

#include "EngineAPI.h"
#include "EventAPI.h"
#include "xrCore/xrCore_benchmark_macros.h"
#include "xrSheduler.h"

class ENGINE_API CEngine
{
public:
    BENCH_SEC_SCRAMBLEMEMBER1
    // DLL api stuff
    CEngineAPI External;
    CEventAPI Event;
    CSheduler Sheduler;
    XRay::Scheduler Scheduler;

    void Initialize();
    void Destroy();

    CEngine();
    ~CEngine();
};

ENGINE_API extern CEngine Engine;
