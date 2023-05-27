// ShaderEditor.cpp : Определяет точку входа для приложения.

#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    if (!IsDebuggerPresent())
        xrDebug::Initialize(pCmdLine);


    Core.Initialize("shader", pCmdLine, LogCallback(ELogCallback, nullptr));
    STools = xr_new<CShaderTool>();
    Tools = STools;

    UI = xr_new<CShaderMain>();
    UI->RegisterCommands();

    UIMainForm *MainForm = xr_new<UIMainForm>();
    ::MainForm = MainForm;
    UI->Push(MainForm, false);

    while (MainForm->Frame())
    {
    }
    
    xr_delete(MainForm);
    Core._destroy();
    return EXIT_SUCCESS;
}
