#include "pch.hpp"
#include "editors/xrECore/Core/ELog.h"
#include "xrCore/Threading/Event.hpp"
#include "xrEngine/main.h"
#include "xrEngine/device.h"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_ioc_cmd.h"

using namespace XRay;
using namespace XRay::Editor;
using namespace XRay::Editor::Controls;
using namespace XRay::Editor::Windows;

Event UICreated;
Event UIThreadExit;
Event ReadyToShowUI;
[System::STAThread]
void UIThreadProc(void*)
{
    System::Windows::Forms::Application::EnableVisualStyles();

    auto windowIDE = gcnew WindowIDE();

    Core.Initialize("OpenXRayEditor", LogCallback(ELogCallback, windowIDE->Log().Handle.ToPointer()), true);

#ifdef XR_X64
    Device.m_sdlWnd = (SDL_Window*)windowIDE->View().GetViewHandle().ToInt64();
#else
    Device.m_sdlWnd = (SDL_Window*)windowIDE->View().GetViewHandle().ToInt32();
#endif
    VERIFY(Device.m_sdlWnd != nullptr);

    UICreated.Set();
    ReadyToShowUI.Wait();
    System::Windows::Forms::Application::Run(windowIDE);
    UIThreadExit.Set();
}

int entry_point(pcstr commandLine)
{
    auto splash = gcnew WindowSplash();
    splash->Show();

    splash->SetStatus("Loading xrDebug...");
    xrDebug::Initialize(false);

    splash->SetStatus("Loading Core...");
    thread_spawn(UIThreadProc, "OpenXRay Editor UI Thread", 0, nullptr);

    UICreated.Wait();
    ReadyToShowUI.Set();

    RunApplication();
//     splash->SetStatus("Loading Settings...");
//     InitSettings();
//     // Adjust player & computer name for Asian
//     if (pSettings->line_exist("string_table", "no_native_input"))
//     {
//         xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
//         xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
//     }
// 
//     FPU::m24r();
// 
//     splash->SetStatus("Loading Engine...");
//     InitEngine();
// 
//     splash->SetStatus("Loading Input...");
//     InitInput();
// 
//     splash->SetStatus("Loading Console...");
//     InitConsole();
// 
//     splash->SetStatus("Creating Renderer List...");
//     Engine.External.CreateRendererList();
// 
//     CCC_LoadCFG_custom cmd("renderer ");
//     cmd.Execute(Console->ConfigFile);

    splash->SetStatus("Loading Engine API...");
    //Engine.External.Initialize();

    splash->SetStatus("Loading Device...");
    //Device.Initialize();
    //Device.Create();

    splash->SetStatus("Loading finished.");
    //ReadyToShowUI.Set();
    splash->Close();

    //Startup();
    UIThreadExit.Wait();

    Core._destroy();
    return 0;
}

int StackoverflowFilter(const int exceptionCode)
{
    if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    int result = 0;
    // BugTrap can't handle stack overflow exception, so handle it here
    __try
    {
        result = entry_point(lpCmdLine);
    }
    __except (StackoverflowFilter(GetExceptionCode()))
    {
        _resetstkoflw();
        FATAL("stack overflow");
    }
    return 0;
}
