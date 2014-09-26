#include "stdafx.h"
#pragma hdrstop

#include "EScenePortalControls.h"
#include "framePortal.h"

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------
__fastcall TUI_ControlPortalSelect::TUI_ControlPortalSelect(int st, int act, ESceneToolBase* parent):TUI_CustomControl(st,act,parent)
{
}
bool __fastcall TUI_ControlPortalSelect::Start(TShiftState Shift)
{
	return SelectStart(Shift);
}
void __fastcall TUI_ControlPortalSelect::Move(TShiftState Shift)
{
	SelectProcess(Shift);
}

bool __fastcall TUI_ControlPortalSelect::End(TShiftState Shift)
{
	return SelectEnd(Shift);
}

