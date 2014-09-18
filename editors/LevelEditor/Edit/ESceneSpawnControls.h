#ifndef ESceneSpawnControlsH
#define ESceneSpawnControlsH

#include "ESceneControlsCustom.h"

//refs
class TfraRPoint;

//---------------------------------------------------------------------------
class TUI_ControlSpawnAdd: public TUI_CustomControl{
    bool __fastcall AppendCallback(SBeforeAppendCallbackParams* p);
public:
    TUI_ControlSpawnAdd(int st, int act, ESceneToolBase* parent);
    virtual ~TUI_ControlSpawnAdd(){;}
	virtual bool Start  (TShiftState _Shift);
};

#endif
