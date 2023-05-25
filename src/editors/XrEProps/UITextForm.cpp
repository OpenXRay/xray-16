#include "stdafx.h"
UITextForm *UITextForm::Form = nullptr;
UITextForm::UITextForm(const char *text) : m_Text(text)
{
    m_Ok = false;
    xr_strcpy(m_EditText, m_Text.c_str());
}

UITextForm::~UITextForm()
{
}

void UITextForm::Draw()
{
    ImGui::BeginGroup();
    if (ImGui::Button("Ok"))
        CLBOk();
    ImGui::SameLine(0);
    if (ImGui::Button("Cancel"))
        CLBCancel();
    ImGui::SameLine(150);

    if (ImGui::Button("Load"))
        CLBLoad();
    ImGui::SameLine(0);
    if (ImGui::Button("Save"))
        CLBSave();
    ImGui::SameLine(0);
    if (ImGui::Button("Clear"))
        CLBClear();

    ImGui::EndGroup();
    ImGui::InputTextMultiline("", m_EditText, sizeof(m_EditText), ImVec2(500, 200));
}

void UITextForm::RunEditor(const char *str)
{
    VERIFY(!Form);
    Form = xr_new<UITextForm>(str);
}

void UITextForm::Update()
{
    if (Form && !Form->IsClosed())
    {
        if (ImGui::BeginPopupModal("TextEditor", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
        {
            Form->Draw();
            ImGui::EndPopup();
        }
    }
}

bool UITextForm::GetResult(bool &change, xr_string &result)
{
    if (Form->IsClosed())
    {
        change = Form->m_Ok;
        result = Form->m_Text;
        xr_delete(Form);
        return true;
    }

    return false;
}

void UITextForm::CLBOk()
{
    m_Text = m_EditText;
    bOpen = false;
    m_Ok = true;
}

void UITextForm::CLBCancel()
{
    bOpen = false;
}

void UITextForm::CLBLoad()
{
    R_ASSERT(0);
}

void UITextForm::CLBSave()
{
    R_ASSERT(0);
}

void UITextForm::CLBClear()
{
    m_EditText[0] = 0;
}
