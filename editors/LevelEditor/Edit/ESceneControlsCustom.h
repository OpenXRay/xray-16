#ifndef ESceneControlsCustomH
#define ESceneControlsCustomH

#include "customobject.h"
//---------------------------------------------------------------------------
// refs
class ESceneToolBase;

struct SBeforeAppendCallbackParams{
	LPVOID 		data;
    xr_string 	name_prefix;
    SBeforeAppendCallbackParams()
    {
    	data 		= 0;
        name_prefix	= "";
    }
};
typedef bool __fastcall (__closure *TBeforeAppendCallback)(SBeforeAppendCallbackParams* p);
typedef bool __fastcall (__closure *TAfterAppendCallback)(TShiftState _Shift, CCustomObject* obj);

class TUI_CustomControl{
protected:
    friend class ESceneToolBase;
    int sub_target;
    int action;
// select
    bool bBoxSelection;
    bool SelectStart	(TShiftState _Shift);
    void SelectProcess	(TShiftState _Shift);
    bool SelectEnd		(TShiftState _Shift);
// add
    bool AddStart		(TShiftState _Shift);
    void AddProcess		(TShiftState _Shift);
    bool AddEnd			(TShiftState _Shift);
// move
	Fvector m_MovingXVector;
	Fvector m_MovingYVector;
	Fvector m_MovingReminder;
    bool 				MovingStart		(TShiftState _Shift);
    void 				MovingProcess	(TShiftState _Shift);
    bool 				MovingEnd		(TShiftState _Shift);
// scale
    bool 				ScaleStart		(TShiftState _Shift);
    void				ScaleProcess	(TShiftState _Shift);
    bool 				ScaleEnd		(TShiftState _Shift);
// rotate
	Fvector 			m_RotateVector;
    float 				m_fRotateSnapAngle;
    
    bool 				RotateStart		(TShiftState _Shift);
    void 				RotateProcess	(TShiftState _Shift);
    bool 				RotateEnd		(TShiftState _Shift);
protected:
	bool				CheckSnapList 	(TShiftState Shift);

    CCustomObject*		DefaultAddObject(TShiftState Shift, TBeforeAppendCallback before=0, TAfterAppendCallback after=0);
    bool				DefaultMovingProcess(TShiftState Shift, Fvector& amount);
public:
    ESceneToolBase*	parent_tool;
public:
    				TUI_CustomControl	(int st, int act, ESceneToolBase* parent);
    virtual 		~TUI_CustomControl	(){;}
	virtual bool 	Start  				(TShiftState _Shift);
	virtual bool 	End    				(TShiftState _Shift);
	virtual void 	Move   				(TShiftState _Shift);
	virtual bool 	HiddenMode			();
    virtual bool 	KeyDown				(WORD Key, TShiftState Shift){return false;}
    virtual bool 	KeyUp  				(WORD Key, TShiftState Shift){return false;}
    virtual bool 	KeyPress			(WORD Key, TShiftState Shift){return false;}
    virtual void 	OnEnter				(){;}
    virtual void 	OnExit				(){;}

    int 			Action				(){return action;}
};
//---------------------------------------------------------------------------
#endif
