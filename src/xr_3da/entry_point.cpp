#include "stdafx.h"
#include "resource.h"
#include "StickyKeyFilter.hpp"

#include "xrEngine/main.h"
#include "xrEngine/splash.h"

int entry_point(pcstr commandLine)
{
    if (strstr(commandLine, "-nosplash") == nullptr)
    {
#ifndef DEBUG
        const bool topmost = strstr(commandLine, "-splashnotop") == nullptr ? true : false;
#else
        constexpr bool topmost = false;
#endif
        splash::show(topmost);
    }

    if (strstr(commandLine, "-dedicated"))
        GEnv.isDedicatedServer = true;

    xrDebug::Initialize(GEnv.isDedicatedServer);

    StickyKeyFilter filter;
    if (!GEnv.isDedicatedServer)
        filter.initialize();

    pcstr fsltx = "-fsltx ";
    string_path fsgame = "";
    if (strstr(commandLine, fsltx))
    {
        const u32 sz = xr_strlen(fsltx);
        sscanf(strstr(commandLine, fsltx) + sz, "%[^ ] ", fsgame);
    }
    Core.Initialize("OpenXRay", nullptr, true, *fsgame ? fsgame : nullptr);

    auto result = RunApplication();

    Core._destroy();

    return result;
}

int StackoverflowFilter(const int exceptionCode)
{
    if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE prevInst, char* commandLine, int cmdShow)
{
    int result = 0;
    // BugTrap can't handle stack overflow exception, so handle it here
    __try
    {
        result = entry_point(commandLine);
    }
    __except (StackoverflowFilter(GetExceptionCode()))
    {
        _resetstkoflw();
        FATAL("stack overflow");
    }
    return result;
}
