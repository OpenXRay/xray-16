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
    m_Properties = xr_new<UILPropertiesFrom>();
    m_Render->SetContextMenuEvent(TOnRenderContextMenu(this, &UIMainForm::DrawContextMenu));
    if (dynamic_cast<CLevelPreferences *>(EPrefs)->OpenObjectList)
    {
        UIObjectList::Show();
    }
    if (!dynamic_cast<CLevelPreferences *>(EPrefs)->OpenProperties)
    {
        m_Properties->Close();
    }
}

UIMainForm::~UIMainForm()
{
    dynamic_cast<CLevelPreferences *>(EPrefs)->OpenProperties = !m_Properties->IsClosed();
    dynamic_cast<CLevelPreferences *>(EPrefs)->OpenObjectList = UIObjectList::IsOpen();
    ClearChooseEvents();
    xr_delete(m_Properties);
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
    m_LeftBar->Draw();
    m_Properties->Draw();
    // ImGui::ShowDemoWindow(&bOpen);
    m_Render->Draw();
}

bool UIMainForm::Frame()
{
    if (UI)
        return UI->Idle();
    return false;
}

void UIMainForm::DrawContextMenu()
{
    if (ImGui::BeginMenu("Edit"))
    {
        if (ImGui::MenuItem("Copy"))
        {
            ExecCommand(COMMAND_COPY);
        }
        if (ImGui::MenuItem("Paste"))
        {
            ExecCommand(COMMAND_PASTE);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Cut"))
        {
            ExecCommand(COMMAND_CUT);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Delete"))
        {
            ExecCommand(COMMAND_DELETE_SELECTION);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Visiblity"))
    {
        if (ImGui::MenuItem("Hide Selected"))
        {
            ExecCommand(COMMAND_HIDE_SEL, FALSE);
        }
        if (ImGui::MenuItem("Hide Unselected"))
        {
            ExecCommand(COMMAND_HIDE_UNSEL);
        }
        if (ImGui::MenuItem("Hide All"))
        {
            ExecCommand(COMMAND_HIDE_ALL, FALSE);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Unhide All"))
        {
            ExecCommand(COMMAND_HIDE_ALL, TRUE);
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Properties"))
    {
        ExecCommand(COMMAND_SHOW_PROPERTIES);
    }
}