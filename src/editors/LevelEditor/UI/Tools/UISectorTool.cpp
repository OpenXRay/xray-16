#include "stdafx.h"

UISectorTool::UISectorTool()
{
    m_Edit = false;
    m_CreateNewMultiple = false;
    m_CreateNewSingle = false;
    m_MeshAdd = true;
    m_BoxPick = false;
}

UISectorTool::~UISectorTool()
{
}
void UISectorTool::Draw()
{
    ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (ImGui::TreeNode("Command"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Button("Validate Sectors", ImVec2(-1, 0)))
            {
                PortalUtils.Validate(true);
            }
            if (ImGui::Button("Capture Volume", ImVec2(-1, 0)))
            {
                CSector *S = PortalUtils.GetSelectedSector();
                if (S)
                {
                    S->CaptureInsideVolume();
                    Scene->UndoSave();
                }
            }
            if (ImGui::Button("Distribute Objects", ImVec2(-1, 0)))
            {
                CSector *S = PortalUtils.GetSelectedSector();
                if (S)
                {
                    S->DistributeInsideObjects();
                    Scene->UndoSave();
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Create Default", ImVec2(-1, 0)))
            {
                CCustomObject *O = Scene->FindObjectByName(DEFAULT_SECTOR_NAME, OBJCLASS_SECTOR);
                if (O)
                    ELog.DlgMsg(mtInformation, "Default sector already present. Remove this and try again.");
                else
                {
                    if (!PortalUtils.CreateDefaultSector())
                        ELog.DlgMsg(mtInformation, "Default can't created.");
                }
            }
            if (ImGui::Button("Remove Default", ImVec2(-1, 0)))
            {
                if (!PortalUtils.RemoveDefaultSector())
                    ELog.DlgMsg(mtInformation, "Default sector not found.");
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
    if (m_Edit)
        ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
    if (m_Edit && ImGui::TreeNode("Edit"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        {
            if (ImGui::Checkbox("Create New Single (From Mesh)", &m_CreateNewSingle))
            {
                m_CreateNewMultiple = false;
            }
            if (ImGui::Checkbox("Create New Multiple (From Object)", &m_CreateNewMultiple))
            {
                m_CreateNewSingle = false;
            }
            ImGui::Separator();
            ImGui::Text("Meshes");
            ImGui::SameLine();
            if (ImGui::RadioButton("M+", m_MeshAdd))
            {
                m_MeshAdd = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("M-", !m_MeshAdd))
            {
                m_MeshAdd = false;
            }
            if (ImGui::Checkbox("Box Pick", &m_BoxPick))
                ;
            if (m_CreateNewSingle || m_CreateNewMultiple)
            {
                m_BoxPick = false;
            }
        }
        ImGui::Separator();
        ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::TreePop();
    }
}
