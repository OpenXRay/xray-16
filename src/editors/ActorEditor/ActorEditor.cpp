// ActorEditor.cpp : Определяет точку входа для приложения.

#include "stdafx.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
    if (!IsDebuggerPresent())
        xrDebug::Initialize(pCmdLine);

    Core.Initialize("actor", pCmdLine, LogCallback(ELogCallback, nullptr), true, "fs.ltx", false, true);

    ATools = xr_new<CActorTools>();
    Tools = ATools;

    UI = xr_new<CActorMain>();
    UI->RegisterCommands();

    UIMainForm *MainForm = xr_new<UIMainForm>();
    ::MainForm = MainForm;
    UI->Push(MainForm, false);

    while (MainForm->Frame())
    {
    }

    xr_delete(MainForm);
    Core._destroy();
    return 0;
}
