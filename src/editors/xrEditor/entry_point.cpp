#include "pch.hpp"
#include "editors/xrECore/Core/ELog.h"

using namespace XRay;
using namespace XRay::Editor;
using namespace XRay::Editor::Controls;
using namespace XRay::Editor::Windows;

int entry_point(pcstr commandLine)
{
    System::Windows::Forms::Application::EnableVisualStyles();
    auto splash = gcnew WindowSplash();
    splash->Show();

    splash->SetStatus("Loading xrDebug...");
    xrDebug::Initialize(false);

    splash->SetStatus("Loading Core...");
    Core.Initialize("OpenXRayEditor", nullptr, true);

    splash->SetStatus("Loading finished.");
    splash->Close();

    auto windowMain = gcnew WindowIDE();
    System::Windows::Forms::Application::Run(windowMain);
    Core._destroy();
    return 0;
}

int StackoverflowFilter(const int exceptionCode)
{
    if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
        return EXCEPTION_EXECUTE_HANDLER;
    return EXCEPTION_CONTINUE_SEARCH;
}

[System::STAThread]
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