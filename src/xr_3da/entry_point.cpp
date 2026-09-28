#include "stdafx.h"

#if defined(XR_PLATFORM_ANDROID)
#include <SDL_main.h>
#endif

#include "xrEngine/x_ray.h"
#include "xrGame/xrGame.h"
#include "Include/xrRender/xrRender.h"

#if !defined(XR_PLATFORM_WINDOWS)
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#endif

// Always request high performance GPU
extern "C"
{
// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
XR_EXPORT u32 NvOptimusEnablement = 0x00000001; // NVIDIA Optimus

// https://gpuopen.com/amdpowerxpressrequesthighperformance/
XR_EXPORT u32 AmdPowerXpressRequestHighPerformance = 0x00000001; // PowerXpress or Hybrid Graphics
}

std::array<RendererModule*, 2> s_render_modules =
{
#ifdef XR_PLATFORM_WINDOWS
    xray::render::render_r4::GetRendererModule(),
#endif
    xray::render::render_gl::GetRendererModule(),
};

struct tracy_raii
{
    ~tracy_raii()
    {
#ifdef TRACY_ENABLE
        tracy::GetProfiler().RequestShutdown();
        while (!tracy::GetProfiler().HasShutdownFinished())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
#endif
    }
};

int entry_point(pcstr commandLine)
{
    tracy_raii raii;
    commandLine = commandLine ? commandLine : "";
#if defined(XR_PLATFORM_ANDROID)
    android_install_crash_handler();
    android_engine_log_early("[android] native entry point entered");
#endif
    // Renderer smoke is deliberately independent from proprietary game data:
    // it exercises the Android window/context/shader path only.
    auto* game = strstr(commandLine, "-nogame") || strstr(commandLine, "-renderer-smoke") ? nullptr : &xrGame;

    CApplication app{ commandLine, game, s_render_modules };

    return app.Run();
}

#if defined(XR_PLATFORM_WINDOWS)
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
#else
int main(int argc, char *argv[])
{
    int result = EXIT_FAILURE;

    try
    {
        char* commandLine = nullptr;
        int i;
        if(argc > 1)
        {
            size_t sum = 1;
            for(i = 1; i < argc; ++i)
                sum += strlen(argv[i]) + 1;

            commandLine = (char*)xr_malloc(sum);
            ZeroMemory(commandLine, sum);

            for(i = 1; i < argc; ++i)
            {
                strcat(commandLine, argv[i]);
                strcat(commandLine, " ");
            }

            result = entry_point(commandLine);

            xr_free(commandLine);
        }
        else
            result = entry_point("");
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

#if defined(XR_PLATFORM_ANDROID)
    // SDL runs main() on a Java-owned native thread. Returning from that
    // thread leaves the dedicated :engine process (and its other threads)
    // alive after the engine has shut down. The launcher is a separate
    // process, so terminate this process once all engine destructors ran.
    _exit(result);
#endif
    return result;
}
#endif
