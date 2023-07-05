#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    if (!IsDebuggerPresent())
        xrDebug::Initialize(pCmdLine);

    Core.Initialize("level", pCmdLine, LogCallback(ELogCallback, nullptr), true, "fs.ltx", false, true);
    xrSE_Factory::initialize();

    LTools = xr_new<CLevelTool>();
    Tools = LTools;
    
    LUI = xr_new<CLevelMain>();
    UI = LUI;
    UI->RegisterCommands();
    
    Scene = xr_new<EScene>();
    UIMainForm *MainForm = xr_new<UIMainForm>();

    ::MainForm = MainForm;
    UI->Push(MainForm, false);

    while (MainForm->Frame()) 
    {
    }

    xr_delete(MainForm);
    xrSE_Factory::destroy();
    Core._destroy();
    return 0;
}
