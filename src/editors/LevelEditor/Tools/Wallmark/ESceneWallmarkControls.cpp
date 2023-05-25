#include "stdafx.h"

// Node Add
TUI_ControlWallmarkAdd::TUI_ControlWallmarkAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlWallmarkAdd::Start(TShiftState Shift)
{
    ESceneWallmarkTool *S = (ESceneWallmarkTool *)parent_tool;

    S->SelectObjects(false);
    wm_cnt = 0;
    if (S->AddWallmark(UI->m_CurrentRStart, UI->m_CurrentRDir))
    {
        wm_cnt++;
        if (!(Shift & ssAlt))
        {
            Scene->UndoSave();
            ResetActionToSelect();
            return false;
        }
        else
            return true;
    }
    return false;
}
void TUI_ControlWallmarkAdd::Move(TShiftState _Shift)
{
}
bool TUI_ControlWallmarkAdd::End(TShiftState _Shift)
{
    if (!(_Shift & ssAlt))
        ResetActionToSelect();
    if (wm_cnt)
        Scene->UndoSave();
    return true;
}

//------------------------------------------------------------------------------------
// WM Move
//------------------------------------------------------------------------------------
TUI_ControlWallmarkMove::TUI_ControlWallmarkMove(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}
bool TUI_ControlWallmarkMove::Start(TShiftState Shift)
{
    if ((Shift & ssCtrl))
    {
        ESceneWallmarkTool *S = (ESceneWallmarkTool *)parent_tool;
        if (S->MoveSelectedWallmarkTo(UI->m_CurrentRStart, UI->m_CurrentRDir))
            Scene->UndoSave();
    }
    return false;
}

void TUI_ControlWallmarkMove::Move(TShiftState _Shift)
{
}

bool TUI_ControlWallmarkMove::End(TShiftState _Shift)
{
    return false;
}
