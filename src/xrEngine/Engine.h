#pragma once

#include "Common/Platform.hpp"

// you must define ENGINE_BUILD then building the engine itself
// and not define it if you are about to build DLL

#include "Engine_impexp.h"
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
