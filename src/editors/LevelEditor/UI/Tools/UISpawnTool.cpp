#include "stdafx.h"

UISpawnTool::UISpawnTool()
{
    m_selPercent = 100;
    m_Current = nullptr;
    m_SpawnList = xr_new<UIItemListForm>();
    m_SpawnList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UISpawnTool::OnItemFocused));
    RefreshList();
    m_AttachObject = false;
}

UISpawnTool::~UISpawnTool()
{
    xr_delete(m_SpawnList);
}

void UISpawnTool::Draw()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Reference Select"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            ImGui::Text("Select by Current: ");
            ImGui::SameLine();
            if (ImGui::Button(" +"))
            {
                SelByRefObject(true);
            }
            ImGui::SameLine();
            if (ImGui::Button(" -"))
            {
                SelByRefObject(false);
            }
            ImGui::Text("Select by Selected:");
            ImGui::SameLine();
            if (ImGui::Button("=%"))
            {
                MultiSelByRefObject(true);
            }
            ImGui::SameLine();
            if (ImGui::Button("+%"))
            {
                MultiSelByRefObject(false);
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-ImGui::GetTextLineHeight() - 8);
            ImGui::DragFloat("%", &m_selPercent, 1, 0, 100, "%.1f");
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Commands"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            float size = float(ImGui::CalcItemWidth());
            {
                if (ImGui::Checkbox("Attach Object...", &m_AttachObject))
                {
                    if (m_AttachObject)
                        ExecCommand(COMMAND_CHANGE_ACTION, etaAdd);
                }
                ImGui::SameLine(0, 10);
                if (ImGui::Button("Detach Object", ImVec2(-1, 0)))
                {
                    ObjectList lst;
                    if (Scene->GetQueryObjects(lst, OBJCLASS_SPAWNPOINT, 1, 1, 0))
                    {
                        for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        {
                            CSpawnPoint *O = dynamic_cast<CSpawnPoint *>(*it);
                            R_ASSERT(O);
                            O->DetachObject();
                        }
                    }
                }
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Object List"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::Separator();
        m_SpawnList->Draw();
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
}

void UISpawnTool::SelByRefObject(bool flag)
{
    ObjectList objlist;
    LPCSTR N = Current();
    if (N)
    {
        ObjectIt _F = Scene->FirstObj(OBJCLASS_SPAWNPOINT);
        ObjectIt _E = Scene->LastObj(OBJCLASS_SPAWNPOINT);
        for (; _F != _E; _F++)
        {
            if ((*_F)->Visible())
            {
                CSpawnPoint *_O = (CSpawnPoint *)(*_F);
                if (_O->RefCompare(N))
                    _O->Select(flag);
            }
        }
    }
}

void UISpawnTool::MultiSelByRefObject(bool clear_prev)
{
    ObjectList objlist;
    LPU32Vec sellist;
    if (Scene->GetQueryObjects(objlist, OBJCLASS_SPAWNPOINT, 1, 1, -1))
    {
        for (ObjectIt it = objlist.begin(); it != objlist.end(); it++)
        {
            LPCSTR N = ((CSpawnPoint *)*it)->RefName();
            ObjectIt _F = Scene->FirstObj(OBJCLASS_SPAWNPOINT);
            ObjectIt _E = Scene->LastObj(OBJCLASS_SPAWNPOINT);
            for (; _F != _E; _F++)
            {
                CSpawnPoint *_O = (CSpawnPoint *)(*_F);
                if ((*_F)->Visible() && _O->RefCompare(N))
                {
                    if (clear_prev)
                    {
                        _O->Select(false);
                        sellist.push_back((u32 *)_O);
                    }
                    else
                    {
                        if (!_O->Selected())
                            sellist.push_back((u32 *)_O);
                    }
                }
            }
        }
        std::sort(sellist.begin(), sellist.end());
        sellist.erase(std::unique(sellist.begin(), sellist.end()), sellist.end());
        std::random_shuffle(sellist.begin(), sellist.end());
        int max_k = iFloor(float(sellist.size()) / 100.f * float(m_selPercent) + 0.5f);
        int k = 0;
        for (LPU32It o_it = sellist.begin(); k < max_k; o_it++, k++)
        {
            CSpawnPoint *_O = (CSpawnPoint *)(*o_it);
            _O->Select(true);
        }
    }
}

void UISpawnTool::RefreshList()
{
    ListItemsVec items;
    LHelper().CreateItem(items, RPOINT_CHOOSE_NAME, 0, 0, RPOINT_CHOOSE_NAME);
    LHelper().CreateItem(items, ENVMOD_CHOOSE_NAME, 0, 0, ENVMOD_CHOOSE_NAME);
    CInifile::Root &data = ((CInifile *)pSettings)->sections();
    for (CInifile::RootIt it = data.begin(); it != data.end(); it++)
    {
        LPCSTR val;
        if ((*it)->line_exist("$spawn", &val))
        {
            shared_str caption = pSettings->r_string_wb((*it)->Name, "$spawn");
            shared_str sect = (*it)->Name;
            if (caption.size())
            {
                ListItem *I = LHelper().CreateItem(items, caption.c_str(), 0, ListItem::flDrawThumbnail, (LPVOID) * (*it)->Name);
                // m_caption_to_sect[caption] = sect;
            }
        }
    }
    m_SpawnList->AssignItems(items);
}

void UISpawnTool::OnItemFocused(ListItem *item)
{
    m_Current = 0;
    if (item)
    {
        m_Current = (LPCSTR)item->m_Object;
    }
    ExecCommand(COMMAND_RENDER_FOCUS);
}
