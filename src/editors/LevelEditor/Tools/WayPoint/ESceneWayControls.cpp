#include "stdafx.h"

void ESceneWayTool::OnActivate()
{
    inherited::OnActivate();
    UIWayTool *frame = (UIWayTool *)pForm;
    if (sub_target == estWayModePoint)
        frame->SetWayMode(false);
    else
        frame->SetWayMode(true);
}

void ESceneWayTool::CreateControls()
{
    inherited::CreateDefaultControls(estWayModeWay);
    inherited::CreateDefaultControls(estWayModePoint);
    AddControl(xr_new<TUI_ControlWayPointAdd>(estWayModePoint, etaAdd, this));
    // frame
    pForm = xr_new<UIWayTool>();
}

void ESceneWayTool::RemoveControls()
{
    inherited::RemoveControls();
}
//----------------------------------------------------

//---------------------------------------------------------------------------
TUI_ControlWayPointAdd::TUI_ControlWayPointAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlWayPointAdd::Start(TShiftState Shift)
{
    ObjectList lst;
    Scene->GetQueryObjects(lst, OBJCLASS_WAY, 1, 1, -1);
    UIWayTool *frame = (UIWayTool *)parent_tool->pForm;
    if (1 != lst.size())
    {
        ELog.DlgMsg(mtInformation, "Select one WayObject.");
        return false;
    }
    Fvector p;
    if (LUI->PickGround(p, UI->m_CurrentRStart, UI->m_CurrentRDir, 1))
    {
        CWayObject *obj = (CWayObject *)lst.front();
        R_ASSERT(obj);
        CWayPoint *last_wp = obj->GetFirstSelected();
        CWayPoint *wp = obj->AppendWayPoint();
        wp->MoveTo(p);
        if (frame->IsAutoLink())
        {
            if (last_wp)
                last_wp->AddSingleLink(wp);
        }
        Scene->UndoSave();
    }
    if (!(Shift & ssAlt))
        ResetActionToSelect();
    return false;
}

void TUI_ControlWayPointAdd::OnEnter()
{
}
