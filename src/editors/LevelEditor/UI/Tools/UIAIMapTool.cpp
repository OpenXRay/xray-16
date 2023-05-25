#include "stdafx.h"

UIAIMapTool::UIAIMapTool()
{
	m_Mode = mdAppend;
	m_AutoLink = true;
	m_IgnoreConstraints = false;
	m_IgnoreMaterialsListSelected = 0;
	m_ChooseIgnoreMaterials = false;
}

UIAIMapTool::~UIAIMapTool()
{
}

void UIAIMapTool::Draw()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Commands"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		{
			if (ImGui::Button("Generate Full", ImVec2(-1, 0)))
			{
				tool->GenerateMap(false);
			}
			if (ImGui::Button("Generate Selected", ImVec2(-1, 0)))
			{
				tool->GenerateMap(true);
			}
			if (ImGui::Button("Clear AI Map", ImVec2(-1, 0)))
			{
				if (ELog.DlgMsg(mtConfirmation, mbYes | mbNo, "Are you sure to clear AI Map?") == mrYes)
				{
					tool->Clear();
					Scene->UndoSave();
				}
			}
		}
		ImGui::Separator();
		{
			if (ImGui::Button("Smooth Selected", ImVec2(-1, 0)))
				tool->SmoothNodes();
			if (ImGui::Button("Reset Selected", ImVec2(-1, 0)))
				tool->ResetNodes();
		}
		ImGui::Separator();
		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::TreePop();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("AI Map Nodes"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		{
			ImGui::Checkbox("Ignore Constraints", &m_IgnoreConstraints);
			ImGui::Checkbox("Auto Link", &m_AutoLink);
		}
		ImGui::Separator();
		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::TreePop();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Ignore materials"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		{
			if (ImGui::Button("Add"))
			{
				UIChooseForm::SelectItem(smGameMaterial, 1, 0, 0, 0, 0, 0, 0);
				m_ChooseIgnoreMaterials = true;
			}
			ImGui::SameLine(0, -1);
			if (ImGui::Button("X"))
			{
				m_IgnoreMaterialsListSelected = 0;
				m_IgnoreMaterialsList.clear();
				tool->m_ignored_materials.clear();
			}
			ImGui::SetNextItemWidth(-1);
			ImGui::ListBox(
				"##mat_list_box", &m_IgnoreMaterialsListSelected, [](void *data, int ind, const char **out) -> bool
				{*out = reinterpret_cast<xr_vector<xr_string>*>(data)->at(ind).c_str();  return true; },
				reinterpret_cast<void *>(&this->m_IgnoreMaterialsList), m_IgnoreMaterialsList.size(), 7);
		}
		ImGui::Separator();
		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::TreePop();
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Link Commands"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::PushItemWidth(-1);
		float size = float(ImGui::CalcItemWidth());
		{
			float my_tex_w = (float)ImGui::GetIO().Fonts->TexWidth;
			float my_tex_h = (float)ImGui::GetIO().Fonts->TexHeight;

			{
				if (ImGui::RadioButton("Add    ", m_Mode == mdAppend))
				{
					m_Mode = mdAppend;
				}
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none1", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				if (ImGui::ArrowButton("Up", ImGuiDir_Up))
				{
					SideClick(1);
				}
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none2", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none3", ImVec2(ImGui::GetFrameHeight() * 2, ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				if (ImGui::Button("Select 0-Link", ImVec2(-1, 0)))
				{
					tool->SelectNodesByLink(0);
					Scene->UndoSave();
				}
			}
			{
				if (ImGui::RadioButton("Delete ", m_Mode == mdRemove))
				{
					m_Mode = mdRemove;
				}
				ImGui::SameLine(0, -1);
				if (ImGui::ArrowButton("Left", ImGuiDir_Left))
				{
					SideClick(0);
				}
				ImGui::SameLine(0, -1);
				if (ImGui::Button("X", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
				{
					SideClick(4);
				}
				ImGui::SameLine(0, -1);
				if (ImGui::ArrowButton("Right", ImGuiDir_Right))
				{
					SideClick(2);
				}
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none4", ImVec2(ImGui::GetFrameHeight() * 2, ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				if (ImGui::Button("Select 1-Link", ImVec2(-1, 0)))
				{
					tool->SelectNodesByLink(1);
					Scene->UndoSave();
				}
			}
			{
				if (ImGui::RadioButton("Invert ", m_Mode == mdInvert))
				{
					m_Mode = mdInvert;
				}
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none5", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				if (ImGui::ArrowButton("Down", ImGuiDir_Down))
				{
					SideClick(3);
				}
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none6", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				ImGui::InvisibleButton("none7", ImVec2(ImGui::GetFrameHeight() * 2, ImGui::GetFrameHeight()));
				ImGui::SameLine(0, -1);
				if (ImGui::Button("Select 2-Link", ImVec2(-1, 0)))
				{
					tool->SelectNodesByLink(2);
					Scene->UndoSave();
				}
			}
		}

		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::TreePop();
	}
}

//---------------------------------------------------------------------------

static const int idx[5][4] = {
	{0, 1, 2, 3},
	{1, 2, 3, 0},
	{2, 3, 0, 1},
	{3, 0, 1, 2},
	{4, 4, 4, 4},
};

int ConvertV2L(int side)
{
	if (side < 4)
	{
		const Fvector &HPB = EDevice.m_Camera.GetHPB();
		float h = angle_normalize(HPB.x) / PI;
		R_ASSERT((h >= 0.f) && (h <= 2.f));
		if (h > 0.25f && h <= 0.75f)
			return idx[3][side];
		else if (h > 0.75f && h <= 1.25f)
			return idx[2][side];
		else if (h > 1.25f && h <= 1.75f)
			return idx[1][side];
		else
			return idx[0][side];
	}
	else
		return side;
}

static const u8 fl[5] = {
	SAINode::flN1, SAINode::flN2, SAINode::flN3, SAINode::flN4,
	SAINode::flN1 | SAINode::flN2 | SAINode::flN3 | SAINode::flN4,
	//    					 	 SAINode::flN1|SAINode::flN2,SAINode::flN2|SAINode::flN3,
	//    					 	 SAINode::flN3|SAINode::flN4,SAINode::flN4|SAINode::flN1
};
void UIAIMapTool::SideClick(int tag)
{
	ESceneAIMapTool::EMode mode;
	switch (m_Mode)
	{
	case UIAIMapTool::mdAppend:
		mode = ESceneAIMapTool::mdAppend;
		break;
	case UIAIMapTool::mdRemove:
		mode = ESceneAIMapTool::mdRemove;
		break;
	case UIAIMapTool::mdInvert:
		mode = ESceneAIMapTool::mdInvert;
		break;
	}

	tool->MakeLinks(fl[ConvertV2L(tag)], mode, m_IgnoreConstraints);
	Scene->UndoSave();
	UI->RedrawScene();
}
//---------------------------------------------------------------------------
void UIAIMapTool::OnDrawUI()
{
	if (m_ChooseIgnoreMaterials)
	{
		bool result = false;
		xr_string name;
		if (UIChooseForm::GetResult(result, name))
		{
			if (result)
			{
				m_IgnoreMaterialsList.push_back(name);
				SGameMtl *mtl = GMLib.GetMaterial(name.c_str());
				tool->m_ignored_materials.push_back(mtl->GetID());
			}
			m_ChooseIgnoreMaterials = false;
		}
		UIChooseForm::Update();
	}
}