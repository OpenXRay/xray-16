#include "stdafx.h"

TUI_ControlGroupAdd::TUI_ControlGroupAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlGroupAdd::AfterAppendCallback(TShiftState Shift, CCustomObject *obj)
{
    bool result = false;
    ESceneGroupTool *ot = dynamic_cast<ESceneGroupTool *>(parent_tool);
    if (ot->GetCurrentObject())
    {
        CGroupObject *group = dynamic_cast<CGroupObject *>(obj);
        R_ASSERT(group);
        LPCSTR short_name = ot->GetCurrentObject();
        result = group->SetReference(short_name);
        if (result)
        {
            string256 namebuffer;
            Scene->GenObjectName(OBJCLASS_GROUP, namebuffer, short_name);
            group->SetName(namebuffer);
        }
    }
    return result;
}
bool TUI_ControlGroupAdd::Start(TShiftState Shift)
{
    DefaultAddObject(Shift, 0, TAfterAppendCallback(this, &TUI_ControlGroupAdd::AfterAppendCallback));
    return false;
}
void TUI_ControlGroupAdd::Move(TShiftState _Shift)
{
}
bool TUI_ControlGroupAdd::End(TShiftState _Shift)
{
    return true;
}
