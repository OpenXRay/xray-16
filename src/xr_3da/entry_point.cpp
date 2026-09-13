#include "stdafx.h"

#include "xrEngine/x_ray.h"
#include "xrGame/xrGame.h"
#include "Include/xrRender/xrRender.h"

#if !defined(XR_PLATFORM_WINDOWS)
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#endif

#if defined(XR_PLATFORM_APPLE)
#include <SDL.h>

namespace
{
bool EnvironmentFlagEnabled(pcstr name)
{
    pcstr value = SDL_getenv(name);
    return value && value[0] && xr_strcmp(value, "0") != 0 && xr_strcmp(value, "false") != 0 &&
        xr_strcmp(value, "off") != 0;
}

std::string GetBundledDefaultCommandLine()
{
    char* basePath = SDL_GetBasePath();
    if (!basePath)
        return {};

    std::string commandLinePath = basePath;
    SDL_free(basePath);
    commandLinePath += "../Resources/openxray/default_command_line.txt";

    FILE* file = fopen(commandLinePath.c_str(), "r");
    if (!file)
        return {};

    char commandLine[1024]{};
    const bool hasCommandLine = fgets(commandLine, sizeof(commandLine), file) != nullptr;
    fclose(file);

    if (!hasCommandLine)
        return {};

    commandLine[strcspn(commandLine, "\r\n")] = '\0';
    return commandLine;
}
} // namespace
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
    auto* game = strstr(commandLine, "-nogame") ? nullptr : &xrGame;

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
#if defined(XR_PLATFORM_APPLE)
        std::string commandLine = GetBundledDefaultCommandLine();
        if (EnvironmentFlagEnabled("OPENXRAY_SKIP_INTRO") && commandLine.find("-nointro") == std::string::npos)
        {
            if (!commandLine.empty())
                commandLine += ' ';
            commandLine += "-nointro";
        }
#else
        std::string commandLine;
#endif

        if (!commandLine.empty())
            commandLine += ' ';

        for (int i = 1; i < argc; ++i)
        {
            commandLine += argv[i];
            commandLine += ' ';
        }

        result = entry_point(commandLine.c_str());
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
