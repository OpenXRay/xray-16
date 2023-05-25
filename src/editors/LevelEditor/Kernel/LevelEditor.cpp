#include "stdafx.h"

class ISE_Abstract;


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    if (!IsDebuggerPresent())
        Debug._initialize(false);

    Core.InitCore("level", ELogCallback);
    XrSE_Factory::initialize();
    
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
    XrSE_Factory::destroy();
    Core.DestroyCore();
    return 0;
}
