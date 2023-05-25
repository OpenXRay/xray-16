#include "stdafx.h"
#include "UIDOOneColor.h"
#include "UIDOShuffle.h"

UIDOOneColor::UIDOOneColor()
{
	list_index = 0;
}

UIDOOneColor::~UIDOOneColor()
{
}

void UIDOOneColor::Draw()
{
	ImGui::BeginChild("ColorIndex", ImVec2(0, ImGui::GetFrameHeight() * 4), false, ImGuiWindowFlags_NoScrollbar);
	ImGui::BeginGroup();

	if (ImGui::ColorEdit3("##value", Color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))	
		DOShuffle->bModif = true;
	
	if (ImGui::Button("X", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
	{
		DOShuffle->bModif = true;
		bOpen = false;
	}

	if (ImGui::Button(">", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
	{
		if (DOShuffle->m_list_selected >= 0 && DOShuffle->m_list_selected < DOShuffle->m_list.size())
		{
			AppendItem(DOShuffle->m_list[DOShuffle->m_list_selected]);
			DOShuffle->bModif = true;
		}
	}

	if (ImGui::Button("<", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
	{
		if (list_index >= 0 && list_index < list.size())
		{
			list.erase(list.begin() + list_index);
			list_index = -1;
			DOShuffle->bModif = true;
		}
	}

	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	ImGui::ListBox(
		"##list", &list_index, [](void *data, int ind, const char **out) -> bool
		{auto item = reinterpret_cast<xr_vector<xr_string>*>(data)->at(ind).c_str();; *out = item; return true; },
		reinterpret_cast<void *>(&list), list.size(), 4);
		
	ImGui::EndChild();
	ImGui::Separator();
}

void UIDOOneColor::RemoveObject(const xr_string &str)
{
	for (auto b = list.begin(), e = list.end(); b != e; b++)
	{
		if (*b == str)
		{
			list.erase(b);
			return;
		}
	}
}

void UIDOOneColor::AppendItem(const xr_string &item)
{
	for (xr_string &i : list)
	{
		if (i == item)
			return;
	}
	list.push_back(item);
}
