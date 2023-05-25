#include "stdafx.h"

UIParticlesTool::UIParticlesTool()
{
    m_Current = nullptr;
    m_ParticlesList = xr_new<UIItemListForm>();
    m_ParticlesList->SetOnItemFocusedEvent(TOnILItemFocused(this, &UIParticlesTool::OnItemFocused));
    ListItemsVec items;
    for (PS::PEDIt E = ::Render->PSLibrary.FirstPED(); E != ::Render->PSLibrary.LastPED(); E++)
    {
        ListItem *I = LHelper().CreateItem(items, *(*E)->m_Name, 0, 0, *E);
        I->SetIcon(1);
    }
    for (PS::PGDIt G = ::Render->PSLibrary.FirstPGD(); G != ::Render->PSLibrary.LastPGD(); G++)
    {
        ListItem *I = LHelper().CreateItem(items, *(*G)->m_Name, 0, 0, *G);
        I->SetIcon(2);
    }

    m_ParticlesList->AssignItems(items);
}

UIParticlesTool::~UIParticlesTool()
{
    xr_delete(m_ParticlesList);
}

void UIParticlesTool::Draw()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Commands"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            ImGui::Text("Ref's Select:   ");
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
            {
                SelByRef(true);
            };
            ImGui::SameLine();
            if (ImGui::Button("-", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
            {
                SelByRef(false);
            };
            ImGui::Text("Selected:       ");
            ImGui::SameLine();
            if (ImGui::ArrowButton("play", ImGuiDir_Right))
            {
                ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
                ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
                for (; _F != _E; _F++)
                {
                    if ((*_F)->Visible() && (*_F)->Selected())
                        ((EParticlesObject *)(*_F))->Play();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("stop", ImVec2(0, ImGui::GetFrameHeight())))
            {
                ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
                ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
                for (; _F != _E; _F++)
                {
                    if ((*_F)->Visible() && (*_F)->Selected())
                        ((EParticlesObject *)(*_F))->Stop();
                }
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Particles"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::Separator();
        m_ParticlesList->Draw();
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
}

void UIParticlesTool::SelByRef(bool flag)
{
    if (m_Current)
    {
        ObjectIt _F = Scene->FirstObj(OBJCLASS_PS);
        ObjectIt _E = Scene->LastObj(OBJCLASS_PS);
        for (; _F != _E; _F++)
        {
            if ((*_F)->Visible())
            {
                EParticlesObject *_O = (EParticlesObject *)(*_F);
                if (_O->RefCompare(m_Current))
                    _O->Select(flag);
            }
        }
    }
}

void UIParticlesTool::OnItemFocused(ListItem *item)
{
    m_Current = nullptr;
    if (item)
    {
        m_Current = item->Key();
    }
}
