#include "stdafx.h"

TUI_ControlPortalSelect::TUI_ControlPortalSelect(int st, int act, ESceneToolBase *parent) : TUI_CustomControl(st, act, parent)
{
}

bool TUI_ControlPortalSelect::Start(TShiftState Shift)
{
	return SelectStart(Shift);
}

void TUI_ControlPortalSelect::Move(TShiftState Shift)
{
	SelectProcess(Shift);
}

bool TUI_ControlPortalSelect::End(TShiftState Shift)
{
	return SelectEnd(Shift);
}
