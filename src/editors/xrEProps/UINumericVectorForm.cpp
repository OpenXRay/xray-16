#include "stdafx.h"

UINumericVectorForm::UINumericVectorForm(const char *title, Fvector *data, Fvector *Reset, int decimal, Fvector *Min, Fvector *Max)
{
	m_Title = title;
	m_Out = data;
	m_Max = Max;
	m_Min = Min;
	m_Reset = Reset;
	m_Edit = *m_Out;
	m_Decimal = decimal;
}

UINumericVectorForm::~UINumericVectorForm()
{
}

void UINumericVectorForm::Draw()
{
	ImGui::Begin(m_Title.c_str(), 0, ImGuiWindowFlags_::ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
	ImGui::BeginGroup();
	ImGui::InputFloat("X", &m_Edit.x, 0.01, 0.1, m_Decimal);
	ImGui::SameLine(0);
	if (ImGui::Button("Ok"))
		CLBOk();
	ImGui::EndGroup();
	ImGui::BeginGroup();
	ImGui::InputFloat("Y", &m_Edit.y, 0.01, 0.1, m_Decimal);
	ImGui::SameLine(0);
	if (ImGui::Button("Cancek"))
		CLBCancel();
	ImGui::EndGroup();
	ImGui::BeginGroup();
	ImGui::InputFloat("Z", &m_Edit.z, 0.01, 0.1, m_Decimal);
	ImGui::SameLine(0);
	if (m_Reset)
		if (ImGui::Button("Reset"))
			CLBReset();
	ImGui::EndGroup();
	ImGui::End();
	if (m_Max)
	{
		if (m_Max->x < m_Edit.x)
			m_Edit.x = m_Max->x;
		if (m_Max->y < m_Edit.y)
			m_Edit.y = m_Max->y;
		if (m_Max->z < m_Edit.z)
			m_Edit.z = m_Max->z;
	}
	if (m_Min)
	{
		if (m_Min->x > m_Edit.x)
			m_Edit.x = m_Min->x;
		if (m_Min->y > m_Edit.y)
			m_Edit.y = m_Min->y;
		if (m_Min->z > m_Edit.z)
			m_Edit.z = m_Min->z;
	}
}

void UINumericVectorForm::CLBOk()
{
	*m_Out = m_Edit;
	bOpen = false;
}

void UINumericVectorForm::CLBCancel()
{
	bOpen = false;
}

void UINumericVectorForm::CLBReset()
{
	m_Edit = *m_Reset;
}
