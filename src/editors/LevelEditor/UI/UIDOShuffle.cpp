#include "stdafx.h"

UIDOShuffle* UIDOShuffle::Form = nullptr;
UIDOShuffle::UIDOShuffle()
{
	bModif = false;
	m_ChooseObject = false;
	m_Texture = nullptr;
	m_TextureNull.create("ed\\ed_nodata");
	m_TextureNull->Load();
	m_Props = xr_new< UIPropertiesForm>();
	m_RealTexture = nullptr;
}

UIDOShuffle::~UIDOShuffle()
{
	m_TextureNull.destroy();
	ApplyChanges();
	xr_delete(m_Props);
	if (m_Texture) m_Texture->Release();
	ClearIndexForms();
	if (m_RealTexture)m_RealTexture->Release();
}

void UIDOShuffle::Draw()
{
	ImGui::Columns(2);
	ImGui::BeginChild("Left");
	{
		VERIFY(m_TextureNull->surface_get());
		if (m_RealTexture != m_Texture)
		{
			if (m_RealTexture)m_RealTexture->Release();
			m_RealTexture = m_Texture;
			if(m_RealTexture) m_RealTexture->AddRef();


		}
		ImGui::Image(m_RealTexture ? m_RealTexture :m_TextureNull->surface_get(), ImVec2(128, 128));

		{
			int selected = m_list_selected;
			ImGui::SetNextItemWidth(-1);
			if (ImGui::ListBox("##list", &selected, [](void* data, int ind, const char** out)->bool {auto item = reinterpret_cast<xr_vector<xr_string>*>(data)->at(ind).c_str();; *out = item; return true; }, reinterpret_cast<void*>(&m_list), m_list.size(), 15))
			{
				if (m_list_selected != selected)
				{
					m_list_selected = selected;
					OnItemFocused(m_list[selected].c_str());
				}
			}
		}
		{
			if (ImGui::Button("+", ImVec2(0, ImGui::GetFrameHeight()))) { UIChooseForm::SelectItem(smObject, 8); m_ChooseObject = true; }; ImGui::SameLine();
			if (ImGui::Button("-", ImVec2(0, ImGui::GetFrameHeight()))) 
			{
				if (m_list_selected >= 0 && m_list.size() > m_list_selected)
				{
					DM->RemoveDO(m_list[m_list_selected].c_str());
					for (UIDOOneColor* one_color : m_color_indices) { one_color->RemoveObject(m_list[m_list_selected]); }
					m_list.erase(m_list.begin() + m_list_selected);
					OnItemFocused(nullptr);
					bModif = true;
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("X", ImVec2(0, ImGui::GetFrameHeight())))
			{
				m_list.clear();
				DM->InvalidateSlots();
				DM->ClearColorIndices();
				ClearIndexForms();
				UI->RedrawScene();
				OnItemFocused(nullptr);
				bModif = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Load..", ImVec2(0, ImGui::GetFrameHeight())))
			{
				xr_string fname;
				if (EFS.GetOpenName(EDevice.m_hWnd,_detail_objects_, fname)) {
					if (DM->ImportColorIndices(fname.c_str())) {
						DM->InvalidateSlots();
						FillData();
						bModif = true;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save..", ImVec2(-1, ImGui::GetFrameHeight())))
			{
				xr_string fname;
				if (EFS.GetSaveName(_detail_objects_, fname)) 
				{
					ApplyChanges(false);
					DM->ExportColorIndices(fname.c_str());
				}
				
			}
		}
		{
			ImGui::BeginChild("Props",  ImVec2(0, 0), false);
			m_Props->Draw();
			ImGui::EndChild();
		}
		
	}
	ImGui::EndChild();
	ImGui::NextColumn();
	ImGui::BeginChild("Right");
	{
		if (ImGui::Button("X")) { ClearIndexForms(); }
		ImGui::SameLine();
		if (ImGui::Button("Append Color Index", ImVec2(-1,0))) 
		{
			m_color_indices.push_back(xr_new<UIDOOneColor>());
			m_color_indices.back()->DOShuffle = this;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		{
			int index = 0;
			
			for (UIDOOneColor* one_color : m_color_indices)
			{
				ImGui::PushID(index);
				one_color->Draw();
				ImGui::PopID();
				index++;
			}
			for (auto b = m_color_indices.begin(), e = m_color_indices.end(); b != e; b++)
			{
				if ((*b)->IsClosed())
				{
					auto ptr = *b;
					xr_delete(ptr);
					m_color_indices.erase(b);
					b = m_color_indices.begin();
				}
			}
		}
		ImGui::PopStyleVar(2);
	}
	ImGui::EndChild();
	ImGui::Columns();
	if (m_ChooseObject)
	{
		bool ok = false;
		xr_vector<xr_string> list;
		if (UIChooseForm::GetResult(ok,list))
		{
			if (ok)
			{
				for (xr_string& l : list)
				{
					if (!FindItem(l.c_str())) 
					{
						if (m_list.size() >= dm_max_objects) {
							ELog.DlgMsg(mtInformation, "Maximum detail objects in scene '%d'", dm_max_objects);
							return;
						}
						DM->AppendDO(l.c_str());
						m_list_selected = m_list.size();
						m_list.push_back(l);
					}
				}
				OnItemFocused(list.back().c_str());
				bModif = true;
			}
			m_ChooseObject = false;
		}
		UIChooseForm::Update();
	}
}

void UIDOShuffle::Update()
{
	if (Form && !Form->IsClosed())
	{
		ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);
		if (ImGui::BeginPopupModal("Detail Object List", &Form->bOpen,0,true))
		{
			Form->Draw();
			ImGui::EndPopup();
		}
	}

}

bool UIDOShuffle::GetResult()
{
	if (!Form->bOpen)
	{
		xr_delete(Form);
		return true;
	}
	return false;
}

void UIDOShuffle::Show(EDetailManager* DM)
{
	VERIFY(!Form);
	Form = xr_new<UIDOShuffle>();
	Form->DM = DM;
	Form->FillData();
}

void UIDOShuffle::FillData()
{
	m_list.clear();
	OnItemFocused(nullptr);
	for (CDetailManager::DetailIt d_it = DM->objects.begin(); d_it != DM->objects.end(); d_it++)
		m_list.push_back(((EDetail*)(*d_it))->GetName());
	ClearIndexForms();
	ColorIndexPairIt S = DM->m_ColorIndices.begin();
	ColorIndexPairIt E = DM->m_ColorIndices.end();
	ColorIndexPairIt it = S;
	for (; it != E; it++) {
		UIDOOneColor* OneColor = xr_new<UIDOOneColor>();
		OneColor->DOShuffle = this;
		m_color_indices.push_back(OneColor);
		OneColor->Color[0] = color_get_R(it->first)/255.f;
		OneColor->Color[1] = color_get_G(it->first) / 255.f;
		OneColor->Color[2] = color_get_B(it->first) / 255.f;
		for (DOIt do_it = it->second.begin(); do_it != it->second.end(); do_it++) 
		{
			EDetail* dd = 0;
			for (CDetailManager::DetailIt d_it = DM->objects.begin(); d_it != DM->objects.end(); d_it++)
				if (0 == strcmp(((EDetail*)(*d_it))->GetName(), (*do_it)->GetName())) { dd = (EDetail*)*d_it; break; }
			VERIFY(dd);
			OneColor->list.push_back(dd->GetName());
		}
	}
}

void UIDOShuffle::OnItemFocused(const char* name)
{

	if (!name)
	{
		if (m_Texture) m_Texture->Release(); m_Texture = nullptr;
		m_Props->ClearProperties();
		m_list_selected = -1;
		return;
	}
	m_Thm = ImageLib.CreateThumbnail(name, EImageThumbnail::ETObject);
	if (m_Texture) m_Texture->Release(); m_Texture = nullptr;
	if (m_Thm)m_Thm->Update(m_Texture);
	if (m_Thm)xr_delete(m_Thm);
	EDetail *dd= DM->FindDOByName(name);
	VERIFY(dd);
	PropItemVec items;

	//PHelper().CreateCaption(items, "Ref Name", dd->GetName());
	PHelper().CreateFloat(items, "Density", &dd->m_fDensityFactor, 0.1f, 1.0f);
	PHelper().CreateFloat(items, "Min Scale", &dd->m_fMinScale, 0.1f, 100.0f);
	PHelper().CreateFloat(items, "Max Scale", &dd->m_fMaxScale, 0.1f, 100.f);
	PHelper().CreateFlag32(items, "No Waving", &dd->m_Flags, DO_NO_WAVING);

	m_Props->AssignItems(items);
}

bool UIDOShuffle::FindItem(const char* name)
{
	for (xr_string& nm : m_list)
	{
		if (nm == name)return true;
	}
	return false;
}

void UIDOShuffle::ClearIndexForms()
{
	for (int i = 0; i < m_color_indices.size(); i++)xr_delete(m_color_indices[i]);
	m_color_indices.clear();
}

bool UIDOShuffle::ApplyChanges(bool msg )
{
	DM->RemoveColorIndices();
	for (u32 k = 0; k < m_color_indices.size(); k++) {
		UIDOOneColor* OneColor = m_color_indices[k];
		if (OneColor->list.size())
		{
			u32 clr = color_rgba_f(OneColor->Color[0], OneColor->Color[1], OneColor->Color[2], 1.f);
			for (xr_string&i:OneColor->list)
				DM->AppendIndexObject(clr, i.c_str(), false);
		}
	}
	if (/*bNeedUpdate||*/bModif&& msg)
	{
		ELog.DlgMsg(mtInformation, "Object or object list changed. Reinitialize needed!");
		DM->InvalidateSlots();
		return true;
	}
	return false;
}

