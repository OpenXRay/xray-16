#include "stdafx.h"

UIRightBarForm::UIRightBarForm()
{
}

UIRightBarForm::~UIRightBarForm()
{
}

void UIRightBarForm::Draw()
{
	ImGui::Begin("RightBar", 0);

	if (ImGui::TreeNode("Custom Object"))
	{
		ImGui::BeginGroup();
		PTools->m_ObjectProps->Draw();
		ImGui::EndGroup();
		ImGui::TreePop();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Item Properties"))
	{
		ImGui::BeginGroup();
		PTools->m_ItemProps->Draw();
		ImGui::EndGroup();
		ImGui::TreePop();
	}

	ImGui::End();
}
