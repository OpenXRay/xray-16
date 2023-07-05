#include "stdafx.h"

UIChooseForm::EventsMap UIChooseForm::m_Events;
UIChooseForm *UIChooseForm::Form = 0;

ImTextureID UIChooseForm::NullTexture = nullptr;

void UIChooseForm::UpdateSelected(UIChooseFormItem *NewSelected)
{
    m_SelectedItem = NewSelected;

    if (!m_SelectedItem)
        return;

    PropItemVec Vec;

    if (E.on_sel)
    {
        E.on_sel(m_SelectedItem->Object, Vec);
        m_Props->AssignItems(Vec);
    }

    if (E.flags.test(SChooseEvents::flClearTexture))
    {
        if (m_Texture)
            m_Texture->Release();

        m_Texture = 0;
    }

    if (!E.on_get_texture.empty())
        E.on_get_texture(m_SelectedItem->Object->name.c_str(), m_Texture);
}

void UIChooseForm::FillItems(u32 choose_id)
{
    m_SelectedItems.clear();
    m_RootItem = UIChooseFormItem("");
    m_RootItem.Form = this;

    m_ChooseID = choose_id;
    int Index = 0;

    for (auto &i : m_Items)
    {
        xr_string Name = i.name.c_str();
        xr_string RealName;

        if (strrchr(i.name.c_str(), '\\'))
            RealName = strrchr(i.name.c_str(), '\\') + 1;
        else
            RealName = i.name.c_str();

        UIChooseFormItem *Item = static_cast<UIChooseFormItem *>(m_RootItem.AppendItem(Name.c_str()));
        Item->Object = &i;
        Item->Text = RealName.c_str();
        Item->Index = Index++;
    }

    if (m_Flags.is(cfAllowNone) && !m_Flags.is(cfMultiSelect))
    {
        m_ItemNone.name = NONE_CAPTION;
        xr_string Name = m_ItemNone.name.c_str();
        UIChooseFormItem *Item = static_cast<UIChooseFormItem *>(m_RootItem.AppendItem(Name.c_str()));
        Item->Object = &m_ItemNone;
        Item->Text = NONE_CAPTION;
        Item->Index = Index++;
    }

    m_RootItem.Sort();
}

void UIChooseForm::CheckFavorite()
{
    m_RootItem.CheckFavorited();
    m_SelectedItems.clear();
    m_RootItem.FillFavorited(m_SelectedItems);
}

UIChooseForm::UIChooseForm() : m_Texture(nullptr), m_SelectedItem(nullptr), m_RootItem(""), m_SelectedList(-1)
{
    m_Props = xr_new<UIPropertiesForm>();
}

UIChooseForm::~UIChooseForm()
{
    if (m_Texture)
        m_Texture->Release();

    if (!E.on_close.empty())
        E.on_close();

    xr_delete(m_Props);
}

void UIChooseForm::Draw()
{
    if (m_SelectedItem && E.flags.test(SChooseEvents::flAnimated))
    {
        if (!E.on_get_texture.empty())
            E.on_get_texture(m_SelectedItem->Object->name.c_str(), m_Texture);
    }

    ImGui::Columns(2);
    {
        {
            ImGui::Text("Find:");
            ImGui::SameLine();
            m_Filter.Draw("##Find", -1);

            if (ImGui::BeginChild("Left", ImVec2(0, 0), false))
            {
                static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX | ImGuiTableFlags_SizingFixedFit;

                if (ImGui::BeginTable("objects", 1, flags))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
                    ImGui::TableHeadersRow();

                    m_RootItem.DrawRoot();
                    ImGui::EndTable();
                }
            }

            ImGui::EndChild();
        }

        ImGui::NextColumn();

        {
            ImGui::BeginChild("Right", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false);
            {
                if (NullTexture || m_Texture)
                {
                    if (m_Texture)
                        ImGui::Image(m_Texture, ImVec2(192, 192));
                    else
                        ImGui::Image(NullTexture, ImVec2(192, 192));
                }
                else
                    ImGui::InvisibleButton("Image", ImVec2(192, 192));

                // ImGui::Image
                ImGui::Separator();

                if (!m_SelectedItem)
                {
                    ImGui::Text("Name:");
                    ImGui::Text("Hit:");
                }
                else
                {
                    ImGui::Text("Name:%s", m_SelectedItem->Object->name.c_str());
                    ImGui::Text("Hit:%s", m_SelectedItem->Object->hint.c_str());
                }

                ImGui::Separator();
            }

            if (!E.on_sel.empty() && !m_Flags.is(cfMultiSelect))
            {
                if (m_SelectedItem)
                {
                    if (ImGui::BeginChild("Props", ImVec2(0, 0)))
                        m_Props->Draw();

                    ImGui::EndChild();
                }
            }

            else if (m_Flags.is(cfMultiSelect))
            {
                if (ImGui::Button("Up"))
                {
                    if (m_SelectedList > 0)
                    {
                        std::swap(m_SelectedItems[m_SelectedList - 1], m_SelectedItems[m_SelectedList]);
                        m_SelectedList = -(m_SelectedList - 1) - 2;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Down"))
                {
                    if (m_SelectedItems.size() > 1 && m_SelectedList < m_SelectedItems.size() - 1)
                    {
                        std::swap(m_SelectedItems[m_SelectedList], m_SelectedItems[m_SelectedList + 1]);
                        m_SelectedList = -(m_SelectedList + 1) - 2;
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Del"))
                {
                    if (m_SelectedItems.size() && m_SelectedList >= 0)
                    {
                        m_SelectedItems.erase(m_SelectedItems.begin() + m_SelectedList);
                        m_SelectedList = -1;
                    }

                    m_RootItem.CheckFavorited(m_SelectedItems);
                    CheckFavorite();
                }

                ImGui::SameLine();

                if (ImGui::Button("Clear List"))
                {
                    m_SelectedItems.clear();
                    m_RootItem.CheckFavorited(m_SelectedItems);
                    CheckFavorite();
                    m_SelectedList = -1; /*  if (E.flags.test(SChooseEvents::flClearTexture) ){ if (m_Texture)m_Texture->Release(); m_Texture = 0; } */
                    ImGui::SameLine();
                }

                if (ImGui::BeginChild("List", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
                {
                    int i = 0;

                    int HereY = -m_SelectedList - 2;

                    if (HereY >= 0)
                        m_SelectedList = HereY;

                    for (auto &item : m_SelectedItems)
                    {
                        if (HereY == i)
                            ImGui::SetScrollHereY();

                        if (ImGui::Selectable(item->name.c_str(), m_SelectedList == i))                        
                            m_SelectedList = i;
                        
                        i++;
                    }
                }

                ImGui::EndChild();
            }

            ImGui::EndChild();

            ImGui::BeginDisabled(!m_Flags.is(cfMultiSelect) && !GetSelectedItem());

            if (ImGui::Button("Ok", ImVec2(100, 0)))
            {
                if (!m_Flags.is(cfMultiSelect))
                {
                    VERIFY(m_SelectedItems.size() == 0);
                    m_SelectedItems.push_back(GetSelectedItem());
                }

                m_Result = R_Ok;
                bOpen = false;
            }

            ImGui::EndDisabled();
            ImGui::SameLine(0);

            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                m_Result = R_Cancel;
                bOpen = false;
            }
        }
    }
}

void UIChooseForm::SetNullTexture(ImTextureID Texture)
{
    NullTexture = Texture;
}

void UIChooseForm::Update()
{
    if (Form && !Form->IsClosed())
    {
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_::ImGuiCond_FirstUseEver);

        if (ImGui::BeginPopupModal("ChooseForm", nullptr, 0, true))
        {
            Form->Draw();
            ImGui::EndPopup();
        }
    }
}

bool UIChooseForm::IsActive()
{
    return Form;
}

bool UIChooseForm::GetResult(bool &change, shared_str &result)
{
    if (!Form)
        return false;

    if (!Form->bOpen)
    {
        if (Form->m_Result == R_Ok)
        {
            if (!Form->m_Flags.test(cfMultiSelect))            
                VERIFY(Form->GetSelectedItem());
            
            xr_string resultTemp;
            int i = 0;

            for (auto &item : Form->m_SelectedItems)
            {
                if (i != 0)                
                    resultTemp.append(",");
                
                resultTemp.append(item->name.c_str());
                i++;
            }

            result = resultTemp.c_str();
            change = true;
            xr_delete(Form);
            return true;
        }

        change = false;
        xr_delete(Form);
        return true;
    }

    return false;
}

SChooseItem *UIChooseForm::GetSelectedItem()
{
    if (m_Flags.test(cfMultiSelect))
    {
        if (m_SelectedItems.size())
            return m_SelectedItems.back();

        return nullptr;
    }

    if (m_SelectedItem)
        return m_SelectedItem->Object;

    return nullptr;
}

bool UIChooseForm::GetResult(bool& change, xr_string& result)
{
	if (Form->bOpen)
		return false;

	if (Form->m_Result == R_Ok)
	{
		int i = 0;

		for (auto& item : Form->m_SelectedItems)
		{
			if (i != 0)			
				result.append(",");
			
			result.append(item->name.c_str());
			i++;
		}

		change = true;
		xr_delete(Form);
		return true;
	}

	change = false;
	xr_delete(Form);
	return true;
}

bool UIChooseForm::GetResult(bool& change, xr_vector<xr_string>& result)
{
	if (Form->bOpen)
		return false;

	if (Form->m_Result == R_Ok)
	{
		int i = 0;

		for (auto& item : Form->m_SelectedItems)
		{
			result.push_back(item->name.c_str());
			i++;
		}

		change = true;
		xr_delete(Form);
		return true;
	}

	change = false;
	xr_delete(Form);
	return true;
}

void UIChooseForm::SelectItem(u32 choose_ID, int sel_cnt, LPCSTR init_name, TOnChooseFillItems item_fill, void *fill_param, TOnChooseSelectItem item_select, ChooseItemVec *items, u32 mask)
{
    VERIFY(!Form);

    Form = xr_new<UIChooseForm>();
    Form->m_Flags.assign(mask);
    Form->m_Flags.set(cfMultiSelect, sel_cnt > 1);

    // fill items
    if (items)
    {
        VERIFY2(item_fill.empty(), "ChooseForm: Duplicate source.");
        Form->m_Items = *items;
        Form->E.Set("Select Item", 0, item_select, 0, 0, 0);
    }
    else if (!item_fill.empty())
    {
        // custom
        Form->E.Set("Select Item", item_fill, item_select, 0, 0, 0);
    }
    else
    {
        SChooseEvents *e = GetEvents(choose_ID);
        VERIFY2(e, "Can't find choose event.");
        Form->E = *e;
    }

    // set & fill
    Form->m_Title = Form->E.caption.c_str();

    if (!Form->E.on_fill.empty())
        Form->E.on_fill(Form->m_Items, fill_param);

    Form->FillItems(choose_ID);

    if (sel_cnt > 1)
    {
        int cnt = _GetItemCount(init_name);
        xr_string result;

        for (int i = 0; i < cnt; i++)
        {
            _GetItem(init_name, i, result);

            if (auto *Item = Form->m_RootItem.FindItem(result.c_str()))
                ((UIChooseFormItem*)Item)->bSelected = true;
        }

        Form->m_RootItem.SelectedToFavorite(true);
        Form->CheckFavorite();
    }
    else
    {
        if (sel_cnt <= 1 && init_name && init_name[0])
        {
            if (auto Item = Form->m_RootItem.FindItem(init_name))
            {
                ((UIChooseFormItem*)Item)->bSelected = true;
                Form->UpdateSelected((UIChooseFormItem*)Item);
                Form->m_RootItem.OpenParentItems(init_name);
            }
        }
    }
}

void UIChooseForm::AppendEvents(u32 choose_ID, LPCSTR caption, TOnChooseFillItems on_fill, TOnChooseSelectItem on_sel, TGetTexture on_thm, TOnChooseClose on_close, u32 flags)
{
    EventsMapIt it = m_Events.find(choose_ID);
    VERIFY(it == m_Events.end());
    m_Events.insert(std::make_pair(choose_ID, SChooseEvents(caption, on_fill, on_sel, on_thm, on_close, flags)));
}

void UIChooseForm::ClearEvents()
{
    NullTexture->Release();
    m_Events.clear();
}

SChooseEvents *UIChooseForm::GetEvents(u32 choose_ID)
{
    EventsMapIt it = m_Events.find(choose_ID);

    if (it != m_Events.end())    
        return &it->second;    
    else
        return nullptr;
}
