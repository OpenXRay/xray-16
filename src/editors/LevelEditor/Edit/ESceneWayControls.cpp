#include "stdafx.h"
#pragma hdrstop

#include "ESceneWayTools.h"
#include "ESceneWayControls.h"
#include "ui_leveltools.h"
#include "FrameWayPoint.h"
#include "WayPoint.h"
#include "scene.h"
#include "ui_levelmain.h"

void ESceneWayTool::OnActivate()
{
	inherited::OnActivate	();
	TfraWayPoint* frame		=(TfraWayPoint*)pFrame;
    if (sub_target==estWayModePoint)	frame->ebModePoint->Down 	= true;
    else								frame->ebModeWay->Down 		= true;
}
//----------------------------------------------------

void ESceneWayTool::CreateControls()
{
	inherited::CreateDefaultControls(estWayModeWay);
	inherited::CreateDefaultControls(estWayModePoint);
    AddControl		(xr_new<TUI_ControlWayPointAdd>	(estWayModePoint,	etaAdd,		this));
	// frame
    pFrame 			= xr_new<TfraWayPoint>((TComponent*)0);
}
//----------------------------------------------------

void ESceneWayTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

//---------------------------------------------------------------------------
__fastcall TUI_ControlWayPointAdd::TUI_ControlWayPointAdd(int st, int act, ESceneToolBase* parent):TUI_CustomControl(st,act,parent){
}

bool __fastcall TUI_ControlWayPointAdd::Start(TShiftState Shift)
{
	ObjectList lst; Scene->GetQueryObjects(lst,OBJCLASS_WAY,1,1,-1);
	TfraWayPoint* frame=(TfraWayPoint*)parent_tool->pFrame;
    if (1!=lst.size()){
        ELog.DlgMsg(mtInformation,"Select one WayObject.");
        return false;
    }
    Fvector p;
    if (LUI->PickGround(p,UI->m_CurrentRStart,UI->m_CurrentRDir,1))
    {
        CWayObject* obj = (CWayObject*)lst.front(); R_ASSERT(obj);
        CWayPoint* last_wp=obj->GetFirstSelected();
        CWayPoint* wp=obj->AppendWayPoint();
        wp->MoveTo(p);
        if (frame->ebAutoLink->Down)
        {
            if (last_wp) last_wp->AddSingleLink(wp);
        }
        Scene->UndoSave();
    }
    if (!Shift.Contains(ssAlt)) ResetActionToSelect();
    return false;
}

void __fastcall TUI_ControlWayPointAdd::OnEnter(){
}

