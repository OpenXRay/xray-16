#include "stdafx.h"

UIWayTool::UIWayTool()
{
    m_WayMode = true;
    m_AutoLink = true;
}

UIWayTool::~UIWayTool()
{
}

void UIWayTool::Draw()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Commands"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::RadioButton("Way Mode", m_WayMode))
            {
                LTools->SetTarget(OBJCLASS_WAY, 0);
                m_WayMode = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Way Point", m_WayMode == false))
            {
                LTools->SetTarget(OBJCLASS_WAY, 1);
                m_WayMode = false;
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Link Command"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Checkbox("Auto Link", &m_AutoLink))
            {
            }
            ImGui::PushItemWidth(-1);
            float size = float(ImGui::CalcItemWidth());
            {
                if (ImGui::Button("Create 1-Link", ImVec2(size / 2, 0)))
                {
                    if (m_WayMode)
                    {
                        ELog.DlgMsg(mtInformation, "Before editing enter Point Mode.");
                        return;
                    }
                    bool bRes = false;
                    ObjectList lst;
                    Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    // remove links
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                    {
                        ((CWayObject *)(*it))->RemoveLink();
                        bRes |= ((CWayObject *)(*it))->Add1Link();
                    }
                    if (bRes)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }
                ImGui::SameLine(0, 2);
                if (ImGui::Button("Convert to 1-Link", ImVec2(size / 2, 0)))
                {
                    ObjectList lst;
                    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        ((CWayObject *)(*it))->Convert1Link();
                    if (cnt)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }

                if (ImGui::Button("Create 2-Link", ImVec2(size / 2, 0)))
                {
                    if (m_WayMode)
                    {
                        ELog.DlgMsg(mtInformation, "Before editing enter Point Mode.");
                        return;
                    }
                    bool bRes = false;
                    ObjectList lst;
                    Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        bRes |= ((CWayObject *)(*it))->Add2Link();
                    if (bRes)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }
                ImGui::SameLine(0, 2);
                if (ImGui::Button("Convert to 2-Link", ImVec2(size / 2, 0)))
                {
                    ObjectList lst;
                    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        ((CWayObject *)(*it))->Convert2Link();
                    if (cnt)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }

                if (ImGui::Button("Invert Link", ImVec2(size / 2, 0)))
                {
                    if (m_WayMode)
                    {
                        ELog.DlgMsg(mtInformation, "Before editing enter Point Mode.");
                        return;
                    }
                    ObjectList lst;
                    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        ((CWayObject *)(*it))->InvertLink();
                    if (cnt)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }
                ImGui::SameLine(0, 2);
                if (ImGui::Button("Remove Link", ImVec2(size / 2, 0)))
                {
                    if (m_WayMode)
                    {
                        ELog.DlgMsg(mtInformation, "Before editing enter Point Mode.");
                        return;
                    }
                    ObjectList lst;
                    int cnt = Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, 0);
                    for (ObjectIt it = lst.begin(); it != lst.end(); it++)
                        ((CWayObject *)(*it))->RemoveLink();
                    if (cnt)
                        Scene->UndoSave();
                    ExecCommand(COMMAND_UPDATE_PROPERTIES);
                }
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
}
