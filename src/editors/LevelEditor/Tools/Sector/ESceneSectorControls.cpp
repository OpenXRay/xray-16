#include "stdafx.h"

// add
TUI_ControlSectorAdd::TUI_ControlSectorAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

void TUI_ControlSectorAdd::OnEnter()
{
    m_Action = saNone;
    UISectorTool *fraSector = (UISectorTool *)parent_tool->pForm;
    VERIFY(fraSector);
    fraSector->ShowEdit();
}

void TUI_ControlSectorAdd::OnExit()
{
    UISectorTool *fraSector = (UISectorTool *)parent_tool->pForm;
    VERIFY(fraSector);
    fraSector->HideEdit();
}

void TUI_ControlSectorAdd::AddMesh()
{
    m_Action = saAddMesh;
    CSector *sector = PortalUtils.GetSelectedSector();
    if (!sector)
        return;
    SRayPickInfo pinf;
    if (Scene->RayPickObject(pinf.inf.range, UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_SCENEOBJECT, &pinf, 0))
        sector->AddMesh(dynamic_cast<CSceneObject *>(pinf.s_obj), pinf.e_mesh);
    else if (Scene->RayPickObject(pinf.inf.range, UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_GROUP, &pinf, 0))
    {
        CSceneObject *so = dynamic_cast<CSceneObject *>(pinf.s_obj);
        if (so)
        {
            sector->AddMesh(so, pinf.e_mesh);
        }
    }
}

void TUI_ControlSectorAdd::DelMesh()
{
    m_Action = saDelMesh;
    CSector *sector = PortalUtils.GetSelectedSector();
    if (!sector)
        return;
    SRayPickInfo pinf;
    if (Scene->RayPickObject(pinf.inf.range, UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_SCENEOBJECT, &pinf, 0))
        sector->DelMesh(dynamic_cast<CSceneObject *>(pinf.s_obj), pinf.e_mesh);
}

bool TUI_ControlSectorAdd::AddSector()
{
    string256 namebuffer;
    Scene->GenObjectName(OBJCLASS_SECTOR, namebuffer);
    CSector *_O = xr_new<CSector>((LPVOID)0, namebuffer);
    SRayPickInfo pinf;
    if (Scene->RayPickObject(pinf.inf.range, UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_SCENEOBJECT, &pinf, 0) &&
        (_O->AddMesh(dynamic_cast<CSceneObject *>(pinf.s_obj), pinf.e_mesh)))
    {
        Scene->SelectObjects(false, OBJCLASS_SECTOR);
        Scene->AppendObject(_O);
        return true;
    }
    else
    {
        xr_delete(_O);
        return false;
    }
}

bool valid_color(u32 clr)
{
    u32 _r = color_get_R(clr);
    u32 _g = color_get_G(clr);
    u32 _b = color_get_B(clr);
    if ((_r == 255) && (_g == 255) && (_b == 255))
        return false;
    if ((_r == 127) && (_g == 127) && (_b == 127))
        return false;
    if ((_r == 0) && (_g == 0) && (_b == 0))
        return false;
    if ((_r == 255) && (_g == 0) && (_b == 0))
        return false;
    if ((_r == 127) && (_g == 0) && (_b == 0))
        return false;
    return true;
}

bool TUI_ControlSectorAdd::AddSectors()
{
    int cnt = 0;
    SRayPickInfo pinf;
    if (Scene->RayPickObject(pinf.inf.range, UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_SCENEOBJECT, &pinf, 0))
    {
        CSceneObject *S = dynamic_cast<CSceneObject *>(pinf.s_obj);
        VERIFY(S);
        EditMeshVec *meshes = S->Meshes();
        for (EditMeshIt it = meshes->begin(); it != meshes->end(); it++)
        {
            string256 namebuffer;
            Scene->GenObjectName(OBJCLASS_SECTOR, namebuffer);
            CSector *_O = xr_new<CSector>((LPVOID)0, namebuffer);
            if (_O->AddMesh(S, *it))
            {
                cnt++;
                u32 clr = 0;
                do
                {
                } while (!valid_color(clr = color_rgba(Random.randI(0, 3) * 255 / 2, Random.randI(0, 3) * 255 / 2, Random.randI(0, 3) * 255 / 2, 0)));
                _O->SetColor(clr);
                Scene->SelectObjects(false, OBJCLASS_SECTOR);
                Scene->AppendObject(_O);
            }
            else
            {
                xr_delete(_O);
            }
        }
    }
    return cnt != 0;
}

bool TUI_ControlSectorAdd::Start(TShiftState Shift)
{
    if (Shift == ssRBOnly)
    {
        ExecCommand(COMMAND_SHOWCONTEXTMENU, OBJCLASS_SECTOR);
        return false;
    }
    UISectorTool *fraSector = (UISectorTool *)parent_tool->pForm;
    VERIFY(fraSector);
    if (fraSector->IsCreateNewSingle())
    {
        if (AddSector() && (!(Shift & ssAlt)))
            fraSector->SetCreateNewSingle(false);
        return false;
    }
    if (fraSector->IsCreateNewMultiple())
    {
        AddSectors();
        return false;
    }
    bool bBoxSelection = fraSector->IsBoxPick();
    if (bBoxSelection)
    {
        UI->EnableSelectionRect(true);
        UI->UpdateSelectionRect(UI->m_StartCp, UI->m_CurrentCp);
        m_Action = saMeshBoxSelection;
        return true;
    }
    else
    {
        if (fraSector->IsMeshAdd())
            AddMesh();
        else
            DelMesh();
        return false;
    }
}

void TUI_ControlSectorAdd::Move(TShiftState _Shift)
{
    switch (m_Action)
    {
    case saAddMesh:
        AddMesh();
        break;
    case saDelMesh:
        DelMesh();
        break;
    case saMeshBoxSelection:
        UI->UpdateSelectionRect(UI->m_StartCp, UI->m_CurrentCp);
        break;
    }
}

bool TUI_ControlSectorAdd::End(TShiftState _Shift)
{
    UISectorTool *fraSector = (UISectorTool *)parent_tool->pForm;
    VERIFY(fraSector);
    CSector *sector = PortalUtils.GetSelectedSector();
    if (sector)
    {
        if (m_Action == saMeshBoxSelection)
        {
            UI->EnableSelectionRect(false);
            Fmatrix matrix;
            CSceneObject *O_ref = NULL;
            CEditableObject *O_lib = NULL;

            CFrustum frustum;
            ObjectList lst;
            if (LUI->SelectionFrustum(frustum))
            {
                ;
                Scene->FrustumPick(frustum, OBJCLASS_SCENEOBJECT, lst);
                for (ObjectIt _F = lst.begin(); _F != lst.end(); _F++)
                {
                    O_ref = (CSceneObject *)(*_F);
                    O_lib = O_ref->GetReference();
                    for (EditMeshIt m_def = O_lib->m_Meshes.begin(); m_def != O_lib->m_Meshes.end(); m_def++)
                    {
                        O_ref->GetFullTransformToWorld(matrix);
                        if ((*m_def)->FrustumPick(frustum, matrix))
                        {
                            if (fraSector->IsMeshAdd())
                                sector->AddMesh(O_ref, *m_def);
                            else if (sector->DelMesh(O_ref, *m_def))
                                break;
                        }
                    }
                }
            }
        }
        switch (m_Action)
        {
        case saAddMesh:
        case saDelMesh:
        case saMeshBoxSelection:
            Scene->UndoSave();
            break;
        }
    }
    m_Action = saNone;
    return true;
}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
TUI_ControlSectorSelect::TUI_ControlSectorSelect(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
    pFrame = 0;
}
void TUI_ControlSectorSelect::OnEnter()
{
    pFrame = (UISectorTool *)parent_tool->pForm;
    VERIFY(pFrame);
}
void TUI_ControlSectorSelect::OnExit()
{
    pFrame = 0;
}
bool TUI_ControlSectorSelect::Start(TShiftState Shift)
{
    bool bRes = SelectStart(Shift);
    //	if(!bBoxSelection) pFrame->OnChange();
    return bRes;
}
void TUI_ControlSectorSelect::Move(TShiftState Shift)
{
    SelectProcess(Shift);
}

bool TUI_ControlSectorSelect::End(TShiftState Shift)
{
    bool bRes = SelectEnd(Shift);
    //	if (bBoxSelection) pFrame->OnChange();
    return bRes;
}
