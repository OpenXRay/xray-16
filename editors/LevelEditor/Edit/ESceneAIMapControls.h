#ifndef ESceneAIMapControlsH
#define ESceneAIMapControlsH

#include "ESceneControlsCustom.h"

// refs
class ESceneAIMapTool;

#define estAIMapNode 	0
//---------------------------------------------------------------------------
class TUI_ControlAIMapNodeAdd: public TUI_CustomControl{
	int 			append_nodes;
public:
    TUI_ControlAIMapNodeAdd(int st, int act, ESceneToolBase* parent);
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
};
//---------------------------------------------------------------------------
class TUI_ControlAIMapNodeMove: public TUI_CustomControl{
public:
    TUI_ControlAIMapNodeMove(int st, int act, ESceneToolBase* parent);
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
};
//---------------------------------------------------------------------------
class TUI_ControlAIMapNodeRotate: public TUI_CustomControl{
public:
    TUI_ControlAIMapNodeRotate(int st, int act, ESceneToolBase* parent);
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
};
//---------------------------------------------------------------------------
#endif //UI_AIMapToolsH
