#pragma once

class TUI_ControlPortalSelect : public TUI_CustomControl
{
public:
	TUI_ControlPortalSelect(int st, int act, ESceneToolBase *parent);
	virtual bool Start(TShiftState _Shift);
	virtual bool End(TShiftState _Shift);
	virtual void Move(TShiftState _Shift);
};
