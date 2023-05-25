#include "stdafx.h"

UILeftBarForm::UILeftBarForm()
{
}

UILeftBarForm::~UILeftBarForm()
{
}

void UILeftBarForm::Draw()
{
	ImGui::Begin("LeftBar", 0);

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Items"))
	{
		ImGui::BeginGroup();
		PTools->m_PList->Draw();
		ImGui::EndGroup();
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Reference List"))
	{
		PTools->DrawReferenceList();
		ImGui::TreePop();
	}

	ImGui::End();
}
