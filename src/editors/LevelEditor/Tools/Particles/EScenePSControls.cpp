#include "stdafx.h"

TUI_ControlPSAdd::TUI_ControlPSAdd(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlPSAdd::AfterAppendCallback(TShiftState Shift, CCustomObject *obj)
{
    EParticlesObject *pg = dynamic_cast<EParticlesObject *>(obj);
    R_ASSERT(pg);
    LPCSTR ref_name = ((UIParticlesTool *)parent_tool->pForm)->Current();
    if (!ref_name)
    {
        ELog.DlgMsg(mtInformation, "Nothing selected.");
        return false;
    }
    if (!pg->Compile(ref_name))
    {
        ELog.DlgMsg(mtInformation, "Can't compile particle system '%s'.", ref_name);
        return false;
    }
    return true;
}

bool TUI_ControlPSAdd::Start(TShiftState Shift)
{
    DefaultAddObject(Shift, 0, TAfterAppendCallback(this, &TUI_ControlPSAdd::AfterAppendCallback));
    return false;
}

void TUI_ControlPSAdd::Move(TShiftState _Shift)
{
}
bool TUI_ControlPSAdd::End(TShiftState _Shift)
{
    return true;
}
