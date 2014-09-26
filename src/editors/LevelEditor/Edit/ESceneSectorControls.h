#ifndef ESceneSectorControlsH
#define ESceneSectorControlsH

#include "ESceneControlsCustom.h"

//---------------------------------------------------------------------------
// refs
class TfraSector;

enum ESectorAction{
    saNone,
    saAddMesh,
    saDelMesh,
    saMeshBoxSelection
};

class TUI_ControlSectorAdd: public TUI_CustomControl{
	ESectorAction 	m_Action;
	bool 			AddSectors();
	bool 			AddSector();
	void 			AddMesh();
	void 			DelMesh();
public:
    TUI_ControlSectorAdd(int st, int act, ESceneToolBase* parent);
    virtual ~TUI_ControlSectorAdd(){;}
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
    virtual void OnEnter();
    virtual void OnExit ();
};

class TUI_ControlSectorSelect: public TUI_CustomControl{
    TfraSector*   	pFrame;
public:
    TUI_ControlSectorSelect(int st, int act, ESceneToolBase* parent);
	virtual bool Start  (TShiftState _Shift);
	virtual bool End    (TShiftState _Shift);
	virtual void Move   (TShiftState _Shift);
    virtual void OnEnter();
    virtual void OnExit ();
};
#endif //UI_SectorToolsH
