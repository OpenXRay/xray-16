#include "stdafx.h"

UILeftBarForm::UILeftBarForm()
{
	m_UseSnapList = false;
	m_SnapListMode = false;
	m_SnapItem_Current = 0;
}

UILeftBarForm::~UILeftBarForm()
{
}

void UILeftBarForm::Draw()
{
	ImGui::Begin("LeftBar", 0);
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Tools"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		static ObjClassID Tools[OBJCLASS_COUNT + 1] = {
			OBJCLASS_SCENEOBJECT,
			OBJCLASS_LIGHT,
			OBJCLASS_SOUND_SRC,
			OBJCLASS_SOUND_ENV, OBJCLASS_GLOW,
			OBJCLASS_SHAPE,
			OBJCLASS_SPAWNPOINT,
			OBJCLASS_WAY,
			OBJCLASS_SECTOR,
			OBJCLASS_PORTAL,
			OBJCLASS_GROUP,
			OBJCLASS_PS,
			OBJCLASS_DO,
			OBJCLASS_AIMAP,
			OBJCLASS_WM,
			OBJCLASS_FOG_VOL,
			OBJCLASS_force_dword};

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		ImGui::Columns(2);
		ImGui::Separator();
		for (u32 i = 0; Tools[i] != OBJCLASS_force_dword; i++)
		{
			u32 id = 0;
			if (i % 2)
				id = ((OBJCLASS_COUNT + 1) / 2) + (i / 2);
			else
				id = (i / 2);
			ESceneToolBase *tool = Scene->GetTool(Tools[id]);
			bool visble = tool->IsVisible();
			ImGui::PushID(tool->ClassName());
			if (ImGui::Checkbox("##value", &visble))
			{
				tool->m_EditFlags.set(ESceneToolBase::flVisible, visble);
				UI->RedrawScene();
			};
			ImGui::SameLine();

			if (ImGui::RadioButton(tool->ClassDesc(), LTools->GetTarget() == Tools[id]))
			{
				ExecCommand(COMMAND_CHANGE_TARGET, Tools[id]);
			}
			ImGui::PopID();
			ImGui::NextColumn();
		}
		ImGui::Columns(1);
		ImGui::Separator();
		ImGui::PopStyleVar(2);
		ImGui::TreePop();

		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
	}
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Snap List"))
	{
		ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
		ImGui::Separator();
		{
			ImGui::BulletText("Commands", ImGuiDir_Left);
			if (ImGui::BeginPopupContextItem("Commands", 1))
			{
				if (ImGui::MenuItem("Make List From Selected"))
				{
					ExecCommand(COMMAND_SET_SNAP_OBJECTS);
				}
				if (ImGui::MenuItem("Select Object From List"))
				{
					ExecCommand(COMMAND_SELECT_SNAP_OBJECTS);
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Add Selected To List"))
				{
					ExecCommand(COMMAND_ADD_SEL_SNAP_OBJECTS);
				}
				if (ImGui::MenuItem("Remove Selected From List"))
				{
					ExecCommand(COMMAND_DEL_SEL_SNAP_OBJECTS);
				}
				ImGui::EndPopup();
			}
			ImGui::OpenPopupOnItemClick("Commands", 0);
		}
		//	ImGui::Checkbox("Enable/Show Snap List", &test);
		ImGui::Checkbox("Enable/Show Snap List", &m_UseSnapList);

		ImGui::Separator();
		ImGui::Checkbox("+/- Mode", &m_SnapListMode);
		ImGui::SameLine(0, 10);
		if (ImGui::Button("X"))
		{
			if (ELog.DlgMsg(mtConfirmation, mbYes | mbNo, "Are you sure to clear snap objects?") == mrYes)
				ExecCommand(COMMAND_CLEAR_SNAP_OBJECTS);
		}
		ImGui::PopStyleVar(2);
		/*

	if (lst&&!lst->empty()){
		int idx=0;
		ObjectIt _F=lst->begin();
		for (;_F!=lst->end(); _F++,idx++){
			AnsiString s; s.sprintf("%d: %s",idx,(*_F)->Name);
			lbSnapList->Items->Add(s);
		}
	}*/
		ObjectList *lst = Scene->GetSnapList(true);

		ImGui::SetNextItemWidth(-1);
		ImGui::ListBox(
			"##snap_list_box", &m_SnapItem_Current, [](void *data, int ind, const char **out) -> bool
			{auto item = reinterpret_cast<ObjectList*>(data)->begin(); std::advance(item, ind); *out = (*item)->GetName(); return true; },
			reinterpret_cast<void *>(lst), lst->size(), 7);
		ImGui::TreePop();
		ImGui::PopStyleVar(2);

		ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
	}
	if (LTools->GetToolForm())
		LTools->GetToolForm()->Draw();
	ImGui::End();
}
