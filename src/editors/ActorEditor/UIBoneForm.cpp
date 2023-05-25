#include "stdafx.h"
UIBoneForm *UIBoneForm::Form = nullptr;
UIBoneForm::UIBoneForm()
{
}

UIBoneForm::~UIBoneForm()
{
}

void UIBoneForm::Draw()
{
	if (!ImGui::BeginPopupModal("BoneForm", &bOpen, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize, true))
	{
		bOpen = false;
		ImGui::CloseCurrentPopup();
		return;
	}
	else
	{
		ImGui::Text("Total bone:%d", m_EditObject->BoneCount());
		{
			ImGui::BeginChild("Part1", ImVec2(300, 300));
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Part #1:(%d B)", m_List[0].size());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-50);
			ImGui::InputText("##name", m_Name[0], sizeof(m_Name[0]));
			ImGui::SameLine();
			if (ImGui::Button("set", ImVec2(-1, 0)))
				Move(0);
			{
				ImGui::BeginChild("List", ImVec2(0, 0), true, ImGuiWindowFlags_None);
				for (int n = 0; n < m_List[0].size(); n++)
					ImGui::Checkbox(m_List[0][n].name.c_str(), &m_List[0][n].select);
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
		ImGui::SameLine();
		{
			ImGui::BeginChild("Part2", ImVec2(300, 300));
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Part #2:(%d B)", m_List[1].size());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-50);
			ImGui::InputText("##name", m_Name[1], sizeof(m_Name[1]));
			ImGui::SameLine();
			if (ImGui::Button("set", ImVec2(-1, 0)))
				Move(1);
			{
				ImGui::BeginChild("List", ImVec2(0, 0), true, ImGuiWindowFlags_None);
				for (int n = 0; n < m_List[1].size(); n++)
					ImGui::Checkbox(m_List[1][n].name.c_str(), &m_List[1][n].select);
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}

		{
			ImGui::BeginChild("Part3", ImVec2(300, 300));
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Part #3:(%d B)", m_List[2].size());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-50);
			ImGui::InputText("##name", m_Name[2], sizeof(m_Name[2]));
			ImGui::SameLine();
			if (ImGui::Button("set", ImVec2(-1, 0)))
				Move(2);
			{
				ImGui::BeginChild("List", ImVec2(0, 0), true, ImGuiWindowFlags_None);
				for (int n = 0; n < m_List[2].size(); n++)
					ImGui::Checkbox(m_List[2][n].name.c_str(), &m_List[2][n].select);
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}
		ImGui::SameLine();
		{
			ImGui::BeginChild("Part4", ImVec2(300, 300));
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Part #4:(%d B)", m_List[3].size());
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-50);
			ImGui::InputText("##name", m_Name[3], sizeof(m_Name[3]));
			ImGui::SameLine();
			if (ImGui::Button("set", ImVec2(-1, 0)))
				Move(3);
			{
				ImGui::BeginChild("List", ImVec2(0, 0), true, ImGuiWindowFlags_None);
				for (int n = 0; n < m_List[3].size(); n++)
					ImGui::Checkbox(m_List[3][n].name.c_str(), &m_List[3][n].select);
				ImGui::EndChild();
			}
			ImGui::EndChild();
		}

		if (ImGui::Button("Ok", ImVec2(70, 0)))
		{
			Save();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(70, 0)))
		{
			bOpen = false;
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine(0, 20);
		if (ImGui::Button("Reset To Default"))
		{
			ToDefault();
		}
		ImGui::SameLine(0, 20);
		if (ImGui::Button("Load From"))
		{
			LoadFrom();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save To"))
		{
			SaveTo();
		}
		ImGui::EndPopup();
	}
}

void UIBoneForm::Update()
{
	if (Form)
	{
		if (Form->IsClosed())
		{
			xr_delete(Form);
		}
		else
		{
			Form->Draw();
		}
	}
}

void UIBoneForm::Show()
{
	if (!ATools->CurrentObject())
	{
		ELog.DlgMsg(mtError, "Scene empty. Load object first.");
		return;
	}
	VERIFY(!Form);
	Form = xr_new<UIBoneForm>();
	Form->m_EditObject = ATools->CurrentObject();
	if (Form->m_EditObject)
	{
		Form->m_BoneParts = &Form->m_EditObject->BoneParts();
		Form->FillBoneParts();
	}
}

void UIBoneForm::Move(int to)
{
	if (!Form->m_EditObject)
		return;
	auto &to_vector = m_List[to];
	for (int i = 0; i < 4; i++)
	{
		if (i == to)
			continue;
		for (auto b = m_List[i].begin(), e = m_List[i].end(); b != e; b++)
		{
			if (b->select)
			{
				b->select = false;
				to_vector.push_back(*b);
				m_List[i].erase(b);
				b = m_List[i].begin();
			}
		}
	}
}

void UIBoneForm::FillBoneParts()
{
	if (!Form->m_EditObject)
		return;

	for (int k = 0; k < 4; k++)
	{
		m_List[k].clear();
		m_Name[k][0] = 0;
	}
	for (BPIt it = m_BoneParts->begin(); it != m_BoneParts->end(); it++)
	{
		xr_strcpy(m_Name[it - m_BoneParts->begin()], it->alias.c_str());
		// E[it - m_BoneParts->begin()]->Text = it->alias.c_str();
		for (RStringVecIt w_it = it->bones.begin(); w_it != it->bones.end(); w_it++)
			m_List[it - m_BoneParts->begin()].push_back(*w_it);
	}
}

void UIBoneForm::Save()
{
	for (int k = 0; k < 4; k++)
	{
		if (m_List[k].size() && !xr_strlen(m_Name[k]))
		{
			ELog.DlgMsg(mtError, "Verify parts name.");
			return;
		}
		for (int i = 0; i < 4; i++)
		{
			if (i == k)
				continue;
			if (!m_List[k].size())
				continue;
			string_path Name[2];
			xr_strcpy(Name[0], m_Name[k]);
			xr_strcpy(Name[1], m_Name[i]);
			_strupr_s(Name[0]);
			_strupr_s(Name[1]);
			if (xr_strcmp(Name[0], Name[1]) == 0)
			{
				ELog.DlgMsg(mtError, "Unique name required.");
				return;
			}
		}
	}

	// verify
	U8Vec b_use(m_EditObject->BoneCount(), 0);
	for (int k = 0; k < 4; k++)
	{
		if (m_List[k].size())
		{
			for (auto node : m_List[k])
			{
				b_use[m_EditObject->FindBoneByNameIdx(node.name.c_str())]++;
			}
		}
	}
	for (U8It u_it = b_use.begin(); u_it != b_use.end(); u_it++)
	{
		if (*u_it != 1)
		{
			ELog.DlgMsg(mtError, "Invalid bone part found (missing or duplicate bones).");
			return;
		}
	}
	// save
	m_BoneParts->clear();
	for (int k = 0; k < 4; k++)
	{
		if (m_List[k].size())
		{
			m_BoneParts->push_back(SBonePart());
			SBonePart &BP = m_BoneParts->back();
			BP.alias = m_Name[k];
			for (auto node : m_List[k])
			{
				BP.bones.push_back(node.name);
			}
		}
	}
	ATools->OnMotionDefsModified();
	bOpen = false;
	ImGui::CloseCurrentPopup();
}

void UIBoneForm::SaveTo()
{
	xr_string temp_fn;
	if (EFS.GetSaveName(_import_, temp_fn))
	{
		CInifile ini(temp_fn.c_str(), FALSE, FALSE, TRUE);
		string64 buff;
		for (int i = 0; i < 4; ++i)
		{
			sprintf(buff, "part_%d", i);
			ini.w_string(buff, "partition_name", m_Name[i]);
			for (auto node : m_List[i])
			{
				ini.w_string(buff, node.name.c_str(), NULL);
			}
		}
	}
}

void UIBoneForm::LoadFrom()
{
	xr_string temp_fn;
	if (EFS.GetOpenName(EDevice.m_hWnd, _import_, temp_fn, false, NULL, 0))
	{
		for (int k = 0; k < 4; k++)
		{
			m_List[k].clear();
			m_Name[k][0] = 0;
		}
		CInifile ini(temp_fn.c_str(), TRUE, TRUE, FALSE);
		string64 buff;
		for (int i = 0; i < 4; ++i)
		{
			sprintf(buff, "part_%d", i);
			xr_strcpy(m_Name[i], ini.r_string(buff, "partition_name"));
			CInifile::Sect &S = ini.r_section(buff);
			CInifile::SectCIt it = S.Data.begin();
			CInifile::SectCIt e = S.Data.end();
			for (; it != e; ++it)
			{
				if (0 != stricmp(it->first.c_str(), "partition_name"))
				{
					m_List[i].push_back(it->first);
				}
			}
		}
	}
}

void UIBoneForm::ToDefault()
{
	for (int k = 0; k < 4; k++)
	{
		m_List[k].clear();
		m_Name[k][0] = 0;
	}
	xr_strcpy(m_Name[0], "default");
	for (BoneIt it = m_EditObject->FirstBone(); it != m_EditObject->LastBone(); it++)
	{
		m_List[0].push_back((*it)->Name());
	}
}
