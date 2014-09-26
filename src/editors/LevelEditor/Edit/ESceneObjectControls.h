#ifndef ESceneObjectControlsH
#define ESceneObjectControlsH

#include "ESceneControlsCustom.h"

//------------------------------------------------------------------------------
class TUI_ControlObjectAdd: public TUI_CustomControl{
public:
    TUI_ControlObjectAdd(int st, int act, ESceneToolBase* parent);
    virtual ~TUI_ControlObjectAdd(){;}
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
};
#endif
