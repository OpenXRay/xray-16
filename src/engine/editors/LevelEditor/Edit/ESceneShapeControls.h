#ifndef ESceneShapeControlsH
#define ESceneShapeControlsH

#include "ESceneControlsCustom.h"

//---------------------------------------------------------------------------
class TUI_ControlShapeAdd: public TUI_CustomControl{
	bool __fastcall AfterAppendCallback(TShiftState _Shift, CCustomObject* obj);
public:
    TUI_ControlShapeAdd(int st, int act, ESceneToolBase* parent);
    virtual ~TUI_ControlShapeAdd(){;}
	virtual bool Start  (TShiftState _Shift);
};

#endif
