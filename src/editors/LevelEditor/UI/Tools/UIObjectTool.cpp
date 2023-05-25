#include "stdafx.h"
#include <thread>
#include <atomic>

static std::atomic<bool> RefreshInProgress;

UIObjectTool::UIObjectTool()
{
    m_selPercent = 0.f;
    m_MultiAppend = false;
    m_PropRandom = false;
    m_Current = nullptr;
    m_RandomAppend = false;
    m_Selection = false;
    m_RealTexture = nullptr;
    m_RemoveTexture = nullptr;
    RefreshInProgress = false;
    m_ObjectList = xr_new<UIItemListForm>();
    m_ObjectList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UIObjectTool::OnItemFocused));
    m_TextureNull.create("ed\\ed_nodata");
    m_TextureNull->Load();
    m_Props = xr_new<UIPropertiesForm>();
    RefreshList();
}

UIObjectTool::~UIObjectTool()
{
    while (RefreshInProgress)
        std::this_thread::yield();

    if (m_RemoveTexture)
        m_RemoveTexture->Release();
    if (m_RealTexture)
        m_RealTexture->Release();
    xr_delete(m_Props);
    m_TextureNull.destroy();
    xr_delete(m_ObjectList);
}

void UIObjectTool::Draw()
{
    if (m_RemoveTexture)
        m_RemoveTexture->Release();
    m_RemoveTexture = nullptr;
    static bool bbool = false;
    float a = 1;
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Commands"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Button("Multiple Append", ImVec2(-1, 0)))
            {
                UIChooseForm::SelectItem(smObject, 512, 0);
                m_MultiAppend = true;
            }
        }
        ImGui::Separator();
        {
            float size = float(ImGui::CalcItemWidth());
            {
                if (ImGui::Checkbox("Random Append", &m_RandomAppend))
                {
                    ParentTools->ActivateAppendRandom(m_RandomAppend);
                }
                ImGui::SameLine(0, 10);
                if (ImGui::Button("Random Props...", ImVec2(-1, 0)))
                {
                    m_PropRandom = true;
                    ParentTools->FillAppendRandomPropertiesBegin();
                }
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
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
    if (ImGui::TreeNode("Surface"))
    {

        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Button("Clear Surface in select", ImVec2(-1, 0)))
            {
                Scene->UndoSave();
                ClearSurface(true);
            }
            if (ImGui::Button("Clear Surface in level", ImVec2(-1, 0)))
            {
                if (ELog.DlgMsg(mtConfirmation, mbYes | mbNo, "Are you sure to reset surface in level?") == mrYes)
                {
                    Scene->UndoSave();
                    ClearSurface(false);
                }
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Current Object"))
    {

        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Button("Select ...", ImVec2(-1, 0)))
            {
                UIChooseForm::SelectItem(smObject, 1, m_Current, 0, 0, 0, 0, 0);
                m_Selection = true;
            }
            if (ImGui::Button("Refresh List", ImVec2(-1, 0)))
            {
                RefreshList();
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Preview"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::Image(m_RealTexture ? m_RealTexture : (m_TextureNull->surface_get()), ImVec2(128, 128));
        ImGui::SameLine();
        ImGui::BeginChild("Props", ImVec2(0, 128));
        m_Props->Draw();
        ImGui::EndChild();
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }

    if (!RefreshInProgress)
    {
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
        if (ImGui::TreeNode("Object List"))
        {
            ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
            ImGui::BeginChild("Object List");
            ImGui::Separator();
            m_ObjectList->Draw();
            ImGui::Separator();
            ImGui::EndChild();
            ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
            ImGui::TreePop();
        }
    }
    else
        ImGui::Text("Loading...");
}

void UIObjectTool::RefreshList()
{
    if (RefreshInProgress)
        return;

    std::thread refreshThread(&UIObjectTool::RefreshListInternal, this);
    refreshThread.detach();
}

void UIObjectTool::RefreshListInternal()
{
    Msg("UIObjectTool refresh started");
    RefreshInProgress = true;
    ListItemsVec items;
    FS_FileSet lst;

    if (Lib.GetObjects(lst))
    {
        FS_FileSetIt it = lst.begin();
        FS_FileSetIt _E = lst.end();
        for (; it != _E; it++)
        {
            xr_string fn;
            ListItem *I = LHelper().CreateItem(items, it->name.c_str(), 0, ListItem::flDrawThumbnail, 0);
        }
    }
    if (m_RealTexture)
        m_RemoveTexture = m_RealTexture;
    m_RealTexture = nullptr;
    m_Props->ClearProperties();
    m_ObjectList->AssignItems(items);
    RefreshInProgress = false;
    Msg("UIObjectTool refresh finished");
}

void UIObjectTool::OnDrawUI()
{
    if (m_Selection)
    {
        bool change = false;
        xr_string lst;
        if (UIChooseForm::GetResult(change, lst))
        {
            if (change)
            {
                m_ObjectList->SelectItem(lst.c_str());
            }
            m_Selection = false;
        }

        UIChooseForm::Update();
    }
    if (m_MultiAppend)
    {
        bool change = false;
        SStringVec lst;
        if (UIChooseForm::GetResult(change, lst))
        {
            if (change)
            {
                Fvector pos = {0.f, 0.f, 0.f};
                Fvector up = {0.f, 1.f, 0.f};
                Scene->SelectObjects(false, OBJCLASS_SCENEOBJECT);

                SPBItem *pb = UI->ProgressStart(lst.size(), "Append object: ");
                for (AStringIt it = lst.begin(); it != lst.end(); it++)
                {
                    string256 namebuffer;
                    Scene->GenObjectName(OBJCLASS_SCENEOBJECT, namebuffer, it->c_str());
                    CSceneObject *obj = xr_new<CSceneObject>((LPVOID)0, namebuffer);
                    CEditableObject *ref = obj->SetReference(it->c_str());
                    if (!ref)
                    {
                        ELog.DlgMsg(mtError, "TfraObject:: Can't load reference object.");
                        xr_delete(obj);
                        return;
                    }
                    obj->MoveTo(pos, up);
                    Scene->AppendObject(obj);
                }
                UI->ProgressEnd(pb);
            }
            m_MultiAppend = false;
        }
        UIChooseForm::Update();
    }
    if (m_PropRandom)
    {
        if (ParentTools->FillAppendRandomPropertiesEnd())
        {
            m_PropRandom = false;
        }
        UIPropertiesModal::Update();
    }
}
void UIObjectTool::OnItemFocused(ListItem *item)
{
    if (m_RealTexture)
        m_RemoveTexture = m_RealTexture;
    m_RealTexture = nullptr;
    m_Props->ClearProperties();
    m_Current = nullptr;
    if (item)
    {
        m_Current = item->Key();
        auto *m_Thm = ImageLib.CreateThumbnail(m_Current, EImageThumbnail::ETObject);
        if (m_Thm)
        {
            m_Thm->Update(m_RealTexture);
            PropItemVec Info;
            m_Thm->FillInfo(Info);
            m_Props->AssignItems(Info);
        }
    }
}

void UIObjectTool::SelByRefObject(bool flag)
{
    LPCSTR N = Current();
    if (N)
    {
        ObjectIt _F = Scene->FirstObj(OBJCLASS_SCENEOBJECT);
        ObjectIt _E = Scene->LastObj(OBJCLASS_SCENEOBJECT);
        for (; _F != _E; _F++)
        {
            if ((*_F)->Visible())
            {
                CSceneObject *_O = (CSceneObject *)(*_F);
                if (_O->RefCompare(N))
                    _O->Select(flag);
            }
        }
    }
}

void UIObjectTool::MultiSelByRefObject(bool clear_prev)
{
    ObjectList objlist;
    LPU32Vec sellist;
    if (Scene->GetQueryObjects(objlist, OBJCLASS_SCENEOBJECT, 1, 1, -1))
    {
        for (ObjectIt it = objlist.begin(); it != objlist.end(); it++)
        {
            LPCSTR N = ((CSceneObject *)*it)->RefName();
            ObjectIt _F = Scene->FirstObj(OBJCLASS_SCENEOBJECT);
            ObjectIt _E = Scene->LastObj(OBJCLASS_SCENEOBJECT);
            for (; _F != _E; _F++)
            {
                CSceneObject *_O = (CSceneObject *)(*_F);
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
            CSceneObject *_O = (CSceneObject *)(*o_it);
            _O->Select(true);
        }
    }
}

void UIObjectTool::ClearSurface(bool selected)
{
    {
        ObjectIt _F = Scene->FirstObj(OBJCLASS_SCENEOBJECT);
        ObjectIt _E = Scene->LastObj(OBJCLASS_SCENEOBJECT);
        for (; _F != _E; _F++)
        {
            if ((*_F)->Visible())
            {
                CSceneObject *_O = (CSceneObject *)(*_F);
                if ((_O->Selected() && _O->Visible()) || !selected)
                {
                    _O->ClearSurface();
                }
            }
        }
    }
}
