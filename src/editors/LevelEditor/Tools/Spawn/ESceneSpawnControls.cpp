#include "stdafx.h"

TUI_ControlSpawnAdd::TUI_ControlSpawnAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlSpawnAdd::AppendCallback(SBeforeAppendCallbackParams *p)
{
    LPCSTR ref_name = ((UISpawnTool *)parent_tool->pForm)->Current();
    if (!ref_name)
    {
        ELog.DlgMsg(mtInformation, "Nothing selected.");
        return false;
    }
    if (Scene->LevelPrefix().c_str())
    {
        p->name_prefix = Scene->LevelPrefix().c_str();
        p->name_prefix += "_";
    }
    p->name_prefix += ref_name;
    p->data = (void *)ref_name;
    return (0 != p->name_prefix.length());
}

bool TUI_ControlSpawnAdd::Start(TShiftState Shift)
{
    UISpawnTool *F = (UISpawnTool *)parent_tool->pForm;
    if (F->IsAttachObject())
    {
        CCustomObject *from = Scene->RayPickObject(UI->ZFar(), UI->m_CurrentRStart, UI->m_CurrentRDir, OBJCLASS_DUMMY, 0, 0);
        if (from->FClassID != OBJCLASS_SPAWNPOINT)
        {
            ObjectList lst;
            int cnt = Scene->GetQueryObjects(lst, OBJCLASS_SPAWNPOINT, 1, 1, 0);
            if (1 != cnt)
                ELog.DlgMsg(mtError, "Select one shape.");
            else
            {
                CSpawnPoint *base = dynamic_cast<CSpawnPoint *>(lst.back());
                R_ASSERT(base);
                if (base->AttachObject(from))
                {
                    if (!(Shift & ssAlt))
                    {
                        F->SetAttachObject(false);
                        ResetActionToSelect();
                    }
                }
                else
                {
                    ELog.DlgMsg(mtError, "Attach impossible.");
                }
            }
        }
        else
        {
            ELog.DlgMsg(mtError, "Attach impossible.");
        }
    }
    else
    {
        DefaultAddObject(Shift, TBeforeAppendCallback(this, &TUI_ControlSpawnAdd::AppendCallback));
    }
    return false;
}
