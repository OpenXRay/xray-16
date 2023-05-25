#include "stdafx.h"

#include "../XrECore/Editor/EditorChooseEvents.h"
UIMainForm *MainForm = nullptr;
UIMainForm::UIMainForm()
{
    EnableReceiveCommands();
    if (!ExecCommand(COMMAND_INITIALIZE, (u32)0, (u32)0))
    {
        FlushLog();
        exit(-1);
    }
    ExecCommand(COMMAND_UPDATE_GRID);
    ExecCommand(COMMAND_RENDER_FOCUS);
    FillChooseEvents();
    m_TopBar = xr_new<UITopBarForm>();
    m_Render = xr_new<UIRenderForm>();
    m_MainMenu = xr_new<UIMainMenuForm>();
    m_LeftBar = xr_new<UILeftBarForm>();
    m_RightBar = xr_new<UIRightBarForm>();
}

UIMainForm::~UIMainForm()
{
    ClearChooseEvents();
    xr_delete(m_RightBar);
    xr_delete(m_LeftBar);
    xr_delete(m_MainMenu);
    xr_delete(m_Render);
    xr_delete(m_TopBar);
    ExecCommand(COMMAND_DESTROY, (u32)0, (u32)0);
}

void UIMainForm::Draw()
{
    m_MainMenu->Draw();
    m_TopBar->Draw();
    m_RightBar->Draw();
    m_LeftBar->Draw();
    m_Render->Draw();
}

bool UIMainForm::Frame()
{
    if (UI)
        return UI->Idle();
    return false;
}
