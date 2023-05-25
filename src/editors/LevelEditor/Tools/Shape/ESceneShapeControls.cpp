#include "stdafx.h"

TUI_ControlShapeAdd::TUI_ControlShapeAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlShapeAdd::AfterAppendCallback(TShiftState Shift, CCustomObject *obj)
{
    CEditShape *shape = dynamic_cast<CEditShape *>(obj);
    R_ASSERT(shape);
    UIShapeTool *F = (UIShapeTool *)parent_tool->pForm;
    if (F->IsSphereMode())
    {
        Fsphere S;
        S.identity();
        shape->add_sphere(S);
        // if (!(Shift&ssAlt)) F->SetSphereMode(false);
        return true;
    }
    else
    {
        Fmatrix M;
        M.identity();
        shape->add_box(M);
        //	if (!(Shift&ssAlt)) F->SetSphereMode(true);
        return true;
    }
    return false;
}

bool TUI_ControlShapeAdd::Start(TShiftState Shift)
{
    UIShapeTool *F = (UIShapeTool *)parent_tool->pForm;
    if (F->IsAttachShape())
    {
        CEditShape *from = dynamic_cast<CEditShape *>(Scene->RayPickObject(UI->ZFar(), UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_SHAPE, 0, 0));
        if (from)
        {
            ObjectList lst;
            int cnt = Scene->GetQueryObjects(lst, OBJCLASS_SHAPE, 1, 1, 0);
            if (1 != cnt)
                ELog.DlgMsg(mtError, "Select one shape.");
            else
            {
                CEditShape *base = dynamic_cast<CEditShape *>(lst.back());
                R_ASSERT(base);
                if (base != from)
                {
                    base->Attach(from);
                    if (!(Shift & ssAlt))
                    {
                        F->SetAttachShape(false);
                        ResetActionToSelect();
                    }
                }
            }
        }
    }
    else
        DefaultAddObject(Shift, 0, TAfterAppendCallback(this, &TUI_ControlShapeAdd::AfterAppendCallback));
    return false;
}
