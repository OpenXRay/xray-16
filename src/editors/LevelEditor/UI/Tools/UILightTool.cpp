#include "stdafx.h"

UILightTool::UILightTool()
{
}

UILightTool::~UILightTool()
{
}

void UILightTool::Draw()
{
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Affect in D3D"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::PushItemWidth(-1);
		float size = float(ImGui::CalcItemWidth());
		{
			if (ImGui::Button("Enable Sel", ImVec2(size / 2, 0)))
				UseInD3D(false, true);
			ImGui::SameLine(0, 2);
			if (ImGui::Button("Enable All", ImVec2(size / 2, 0)))
				UseInD3D(true, true);

			if (ImGui::Button("Disable Sel", ImVec2(size / 2, 0)))
				UseInD3D(false, false);
			ImGui::SameLine(0, 2);
			if (ImGui::Button("Disable All", ImVec2(size / 2, 0)))
				UseInD3D(true, false);
		}

		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::TreePop();
	}
}

void UILightTool::UseInD3D(bool bAll, bool bFlag)
{
	ObjectIt _F = Scene->FirstObj(OBJCLASS_LIGHT);
	ObjectIt _E = Scene->LastObj(OBJCLASS_LIGHT);
	for (; _F != _E; _F++)
	{
		CLight *L = (CLight *)*_F;
		if (bAll)
		{
			L->AffectD3D(bFlag);
		}
		else
		{
			if (L->Selected() && L->Visible())
				L->AffectD3D(bFlag);
		}
	}
	UI->RedrawScene();
}
