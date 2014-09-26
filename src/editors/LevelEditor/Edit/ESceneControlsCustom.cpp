#include "stdafx.h"
#pragma hdrstop

#include "ESceneControlsCustom.h"
#include "ui_leveltools.h"
#include "scene.h"
#include "bottombar.h"
#include "ui_levelmain.h"
#include "leftbar.h"

TUI_CustomControl::TUI_CustomControl(int st, int act, ESceneToolBase* parent){
	parent_tool		= parent; VERIFY(parent);
	sub_target		= st;
    action			= act;
	bBoxSelection	= false;
}

bool TUI_CustomControl::Start  (TShiftState _Shift){
	switch(action){
	case etaSelect: return SelectStart(_Shift);
	case etaAdd: 	return AddStart(_Shift);
	case etaMove: 	return MovingStart(_Shift);
	case etaRotate: return RotateStart(_Shift);
	case etaScale: 	return ScaleStart(_Shift);
    }
    return false;
}
bool TUI_CustomControl::End    (TShiftState _Shift){
	switch(action){
	case etaSelect: return SelectEnd(_Shift);
	case etaAdd: 	return AddEnd(_Shift);
	case etaMove: 	return MovingEnd(_Shift);
	case etaRotate: return RotateEnd(_Shift);
	case etaScale: 	return ScaleEnd(_Shift);
    }
    return false;
}
void TUI_CustomControl::Move   (TShiftState _Shift){
	switch(action){
	case etaSelect:	SelectProcess(_Shift); break;
	case etaAdd: 	AddProcess(_Shift);    break;
	case etaMove: 	MovingProcess(_Shift); break;
	case etaRotate:	RotateProcess(_Shift); break;
	case etaScale: 	ScaleProcess(_Shift);  break;
    }
}
bool TUI_CustomControl::HiddenMode(){
	switch(action){
	case etaSelect:	return false;
	case etaAdd: 	return false;
	case etaMove: 	return true;
	case etaRotate:	return true;
	case etaScale: 	return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// add
//------------------------------------------------------------------------------
CCustomObject* __fastcall TUI_CustomControl::DefaultAddObject(TShiftState Shift, TBeforeAppendCallback before, TAfterAppendCallback after)
{
    if (Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return 0;}
    Fvector p,n;
    CCustomObject* obj=0;
    if (LUI->PickGround(p,UI->m_CurrentRStart,UI->m_CurrentRDir,1,&n))
    {
		// before callback
    	SBeforeAppendCallbackParams P;
    	if (before&&!before(&P)) return 0;

		string256 namebuffer;
        Scene->GenObjectName(parent_tool->ClassID, namebuffer, P.name_prefix.c_str());
		obj=Scene->GetOTool(parent_tool->ClassID)->CreateObject(P.data, namebuffer);
        if (!obj->Valid()){
        	xr_delete(obj);
            return 0;
        }
        // after callback
    	if (after&&!after(Shift, obj)){
        	xr_delete(obj);
            return 0;
        }
		obj->MoveTo(p,n);
        Scene->SelectObjects(false,parent_tool->ClassID);
		Scene->AppendObject(obj);
		if (Shift.Contains(ssCtrl)) 
        	ExecCommand(COMMAND_SHOW_PROPERTIES);
        if (!Shift.Contains(ssAlt)) 
        	ResetActionToSelect();
    }
    return obj;
}

bool __fastcall TUI_CustomControl::AddStart(TShiftState Shift)
{
	DefaultAddObject(Shift,0);
    return false;
}
void __fastcall TUI_CustomControl::AddProcess(TShiftState _Shift)
{
}
bool __fastcall TUI_CustomControl::AddEnd(TShiftState _Shift)
{
    return true;
}

bool TUI_CustomControl::CheckSnapList(TShiftState Shift)
{
	if (fraLeftBar->ebSnapListMode->Down){
	    CCustomObject* O=Scene->RayPickObject(UI->ZFar(),UI->m_CurrentRStart,UI->m_CurrentRDir,OBJCLASS_SCENEOBJECT,0,0);
        if (O){
            if (Scene->FindObjectInSnapList(O)){
                if (Shift.Contains(ssAlt)){
                    Scene->DelFromSnapList(O);
                }else if (Shift.Contains(ssCtrl)){
                    Scene->DelFromSnapList(O);
                }
            }else{
                if (!Shift.Contains(ssCtrl)&&!Shift.Contains(ssAlt)){
                    Scene->AddToSnapList(O);
                }else if (Shift.Contains(ssCtrl)){
                    Scene->AddToSnapList(O);
                }
            }
	        return true;
        }else{
        	return false;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// total select
//------------------------------------------------------------------------------
bool __fastcall TUI_CustomControl::SelectStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();

	if (CheckSnapList(Shift)) return false;
    if (Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return false;}
    if (!(Shift.Contains(ssCtrl)||Shift.Contains(ssAlt))) Scene->SelectObjects( false, cls);

    int cnt 		= Scene->RaySelect(Shift.Contains(ssCtrl)?-1:Shift.Contains(ssAlt)?0:1,parent_tool->ClassID);
    bBoxSelection    = ((0!=cnt) && (Shift.Contains(ssCtrl)||Shift.Contains(ssAlt))) || (0==cnt);
    if( bBoxSelection ){
        UI->EnableSelectionRect( true );
        UI->UpdateSelectionRect(UI->m_StartCp,UI->m_CurrentCp);
        return true;
    }
    return false;
}

void __fastcall TUI_CustomControl::SelectProcess(TShiftState _Shift){
    if (bBoxSelection) UI->UpdateSelectionRect(UI->m_StartCp,UI->m_CurrentCp);
}

bool __fastcall TUI_CustomControl::SelectEnd(TShiftState _Shift)
{
    if (bBoxSelection){
        UI->EnableSelectionRect( false );
        bBoxSelection = false;
        Scene->FrustumSelect(_Shift.Contains(ssAlt)?0:1,LTools->CurrentClassID());
    }
    return true;
}

//------------------------------------------------------------------------------------
// moving
//------------------------------------------------------------------------------------
bool __fastcall TUI_CustomControl::MovingStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();

    if(Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return false;}
    if(Scene->SelectionCount(true,cls)==0) return false;

    if (Shift.Contains(ssCtrl)){
        ObjectList lst;
        if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(),1,1,0)){
        	if (lst.size()==1){
                Fvector p,n;
                UI->IR_GetMousePosReal(EDevice.m_hRenderWnd, UI->m_CurrentCp);
                EDevice.m_Camera.MouseRayFromPoint(UI->m_CurrentRStart,UI->m_CurrentRDir,UI->m_CurrentCp);
                if (LUI->PickGround(p,UI->m_CurrentRStart,UI->m_CurrentRDir,1,&n)){
                    for(ObjectIt _F = lst.begin();_F!=lst.end();_F++) (*_F)->MoveTo(p,n);
                    Scene->UndoSave();
                }
            }else{
                Fvector p,n;
                Fvector D={0,-1,0};
                for(ObjectIt _F = lst.begin();_F!=lst.end();_F++){ 
                	if (LUI->PickGround(p,(*_F)->PPosition,D,1,&n)){
	                	(*_F)->MoveTo(p,n);
                    }
                }
            }
        }
        return false;
    }else{
        if (etAxisY==Tools->GetAxis()){
            m_MovingXVector.set(0,0,0);
            m_MovingYVector.set(0,1,0);
        }else{
            m_MovingXVector.set( EDevice.m_Camera.GetRight() );
            m_MovingXVector.y = 0;
            m_MovingYVector.set( EDevice.m_Camera.GetDirection() );
            m_MovingYVector.y = 0;
            m_MovingXVector.normalize_safe();
            m_MovingYVector.normalize_safe();
        }
        m_MovingReminder.set(0,0,0);
    }
    return true;
}

bool __fastcall TUI_CustomControl::DefaultMovingProcess(TShiftState Shift, Fvector& amount)
{
    if (Shift.Contains(ssLeft)||Shift.Contains(ssRight))
    {
        amount.mul( m_MovingXVector, UI->m_MouseSM * UI->m_DeltaCpH.x );
        amount.mad( amount, m_MovingYVector, -UI->m_MouseSM * UI->m_DeltaCpH.y );

        if( Tools->GetSettings(etfMSnap) )
        {
        	CHECK_SNAP(m_MovingReminder.x,amount.x,Tools->m_MoveSnap);
        	CHECK_SNAP(m_MovingReminder.y,amount.y,Tools->m_MoveSnap);
        	CHECK_SNAP(m_MovingReminder.z,amount.z,Tools->m_MoveSnap);
        }

        if (!(etAxisX==Tools->GetAxis())&&!(etAxisZX==Tools->GetAxis())) 	amount.x = 0.f;
        if (!(etAxisZ==Tools->GetAxis())&&!(etAxisZX==Tools->GetAxis())) 	amount.z = 0.f;
        if (!(etAxisY==Tools->GetAxis())) 									amount.y = 0.f;

        return (amount.square_magnitude()>EPS_S);
	}
    return false;
}

void __fastcall TUI_CustomControl::MovingProcess(TShiftState _Shift)
{
	Fvector amount;
	if (DefaultMovingProcess(_Shift,amount))
    {
        ObjectList lst;
        if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(),1,1,0))
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++) (*_F)->Move(amount);
    }
}

bool __fastcall TUI_CustomControl::MovingEnd(TShiftState _Shift)
{
	Scene->UndoSave();
    return true;
}

//------------------------------------------------------------------------------------
// rotate
//------------------------------------------------------------------------------------
bool __fastcall TUI_CustomControl::RotateStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();

    if(Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return false;}
    if(Scene->SelectionCount(true,cls)==0) return false;

    m_RotateVector.set(0,0,0);
    if (etAxisX==Tools->GetAxis()) 		m_RotateVector.set(1,0,0);
    else if (etAxisY==Tools->GetAxis()) m_RotateVector.set(0,1,0);
    else if (etAxisZ==Tools->GetAxis()) m_RotateVector.set(0,0,1);
	m_fRotateSnapAngle = 0;
    return true;
}

void __fastcall TUI_CustomControl::RotateProcess(TShiftState _Shift)
{
    if (_Shift.Contains(ssLeft))
    {
        float amount = -UI->m_DeltaCpH.x * UI->m_MouseSR;

        if( Tools->GetSettings(etfASnap) ) CHECK_SNAP(m_fRotateSnapAngle,amount,Tools->m_RotateSnapAngle);

        ObjectList lst;
        if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(),1,1,0))
            for(ObjectIt _F = lst.begin();_F!=lst.end();_F++)
                if( Tools->GetSettings(etfCSParent) )
                {
                    (*_F)->RotateParent( m_RotateVector, amount );
                } else {
                    (*_F)->RotateLocal( m_RotateVector, amount );
                }
    }
}
bool __fastcall TUI_CustomControl::RotateEnd(TShiftState _Shift)
{
	Scene->UndoSave();
    return true;
}

//------------------------------------------------------------------------------
// scale
//------------------------------------------------------------------------------
bool __fastcall TUI_CustomControl::ScaleStart(TShiftState Shift)
{
	ObjClassID cls = LTools->CurrentClassID();
    if(Shift==ssRBOnly){ ExecCommand(COMMAND_SHOWCONTEXTMENU,parent_tool->ClassID); return false;}
    if(Scene->SelectionCount(true,cls)==0) return false;
	return true;
}

void __fastcall TUI_CustomControl::ScaleProcess(TShiftState _Shift)
{
	float dy = UI->m_DeltaCpH.x * UI->m_MouseSS;
    if (dy>1.f) dy=1.f; else if (dy<-1.f) dy=-1.f;

	Fvector amount;
	amount.set( dy, dy, dy );

    if (Tools->GetSettings(etfNUScale)){
        if (!(etAxisX==Tools->GetAxis())&&!(etAxisZX==Tools->GetAxis())) amount.x = 0.f;
        if (!(etAxisZ==Tools->GetAxis())&&!(etAxisZX==Tools->GetAxis())) amount.z = 0.f;
        if (!(etAxisY==Tools->GetAxis())) amount.y = 0.f;
    }

    ObjectList lst;
    if (Scene->GetQueryObjects(lst,LTools->CurrentClassID(),1,1,0))
        for(ObjectIt _F = lst.begin();_F!=lst.end();_F++) (*_F)->Scale( amount );
}
bool __fastcall TUI_CustomControl::ScaleEnd(TShiftState _Shift)
{
	Scene->UndoSave();
    return true;
}
//------------------------------------------------------------------------------



