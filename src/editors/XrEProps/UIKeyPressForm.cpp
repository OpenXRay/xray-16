#include "stdafx.h"
UIKeyPressForm *UIKeyPressForm::Form = nullptr;
UIKeyPressForm::UIKeyPressForm()
{
	m_Ok = true;
}

UIKeyPressForm::~UIKeyPressForm()
{
}

void UIKeyPressForm::Draw()
{
	if (!ImGui::BeginPopupModal("PressKey", 0, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove, true))
	{
		ImGui::EndPopup();
		return;
	}
	if (!bOpen)
	{
		ImGui::CloseCurrentPopup();
		return;
	}
	if (fmod(m_TimeGlobal * 1000, 1000.f) > 500.f)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "PRESS ANY KEY!!!");
	}
	else
	{
		ImGui::Text("PRESS ANY KEY!!!");
	}
	if (ImGui::Button("Cancel", ImVec2(-1, 0)))
	{
		m_Ok = false;
		bOpen = false;
	}
	ImGui::EndPopup();
	/*else
	{
		;
		ImGui::CloseCurrentPopup();
	}*/
}

void UIKeyPressForm::Update(float timeGlobal)
{
	if (Form)
	{
		Form->m_TimeGlobal = timeGlobal;
		Form->Draw();
	}
}

void UIKeyPressForm::Show()
{
	VERIFY(!Form);
	Form = xr_new<UIKeyPressForm>();
}

bool UIKeyPressForm::SetResult(const xr_shortcut &result)
{
	if (Form && !Form->IsClosed())
	{
		Form->m_Resutl = result;
		Form->bOpen = false;
		return true;
	}
	return false;
}

bool UIKeyPressForm::GetResult(bool &Ok, xr_shortcut &result)
{
	if (Form && Form->IsClosed())
	{
		Ok = Form->m_Ok;
		result = Form->m_Resutl;
		xr_delete(Form);
		return true;
	}
	return false;
}
