#include "stdafx.h"
#include "resource.h"
#if defined(WINDOWS)
#include "StickyKeyFilter.hpp"
#elif defined(LINUX)
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#endif
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

    xrDebug::Initialize();
#ifdef WINDOWS
    StickyKeyFilter filter;
    if (!GEnv.isDedicatedServer)
        filter.initialize();
#endif

    pcstr fsltx = "-fsltx ";
    string_path fsgame = "";
    if (strstr(commandLine, fsltx))
    {
        const u32 sz = xr_strlen(fsltx);
        sscanf(strstr(commandLine, fsltx) + sz, "%[^ ] ", fsgame);
    }
    Core.Initialize("OpenXRay", commandLine, nullptr, true, *fsgame ? fsgame : nullptr);

    auto result = RunApplication();

    Core._destroy();

    return result;
}

#if defined(WINDOWS)
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
#elif defined(LINUX)
int main(int argc, char *argv[])
{
    int result = EXIT_FAILURE;

    try
    {
        char* commandLine = "";
        int i;
        if(argc > 1)
        {
            size_t sum = 0;
            for(i = 1; i < argc; ++i)
                sum += strlen(argv[i]) + 1;

            commandLine = (char*)malloc(sum);
            memset(commandLine, 0, sum);

            for(i = 1; i < argc; ++i)
            {
                strcat(commandLine, argv[i]);
                strcat(commandLine, " ");
            }
        }

        result = entry_point(commandLine);

        free(commandLine);
    }
    catch (const std::overflow_error& e)
    {
        _resetstkoflw();
        FATAL_F("stack overflow: %s", e.what());
    }
    catch (const std::runtime_error& e)
    {
        FATAL_F("runtime error: %s", e.what());
    }
    catch (const std::exception& e)
    {
        FATAL_F("exception: %s", e.what());
    }
    catch (...)
    {
    // this executes if f() throws std::string or int or any other unrelated type
    }

    return result;
}
#endif
