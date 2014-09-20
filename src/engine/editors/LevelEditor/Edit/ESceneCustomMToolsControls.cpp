#include "stdafx.h"
#pragma hdrstop

#include "ESceneCustomMTools.h"
#include "ESceneControlsCustom.h"
#include "ui_leveltools.h"

void ESceneToolBase::CreateDefaultControls(u32 sub_target_id)
{
    for (int a=0; a<etaMaxActions; a++)
    	AddControl(xr_new<TUI_CustomControl>(sub_target_id,a,this));
}

void ESceneToolBase::RemoveControls()
{
    // controls
	for (ControlsIt it=m_Controls.begin(); it!=m_Controls.end(); it++) 
    	xr_delete	(*it);
    m_Controls.clear();
    xr_delete		(pFrame);
}

void ESceneToolBase::AddControl(TUI_CustomControl* c)
{
    R_ASSERT(c);
	for (ControlsIt it=m_Controls.begin(); it!=m_Controls.end(); it++){
    	if (((*it)->sub_target==c->sub_target)&&((*it)->action==c->action)){ 
			xr_delete(*it);
            m_Controls.erase(it);
        	break;
        }
    }
    m_Controls.push_back(c);
}

TUI_CustomControl* ESceneToolBase::FindControl(int subtarget, int action)
{
	if (action==-1) return 0;
	for (ControlsIt it=m_Controls.begin(); it!=m_Controls.end(); it++)
    	if (((*it)->sub_target==subtarget)&&((*it)->action==action)) return *it;
    return 0;
}

void ESceneToolBase::UpdateControl()
{
    if (pCurControl) pCurControl->OnExit();
    pCurControl=FindControl(sub_target,action);
    if (pCurControl) pCurControl->OnEnter();
}

void ESceneToolBase::OnActivate  ()
{
    if (pCurControl) 	pCurControl->OnEnter();

    ExecCommand			(COMMAND_CHANGE_ACTION,etaSelect,estDefault);
    SetChanged			(TRUE);
}

void ESceneToolBase::OnDeactivate()
{
    if (pCurControl) 	pCurControl->OnExit();
}

void ESceneToolBase::SetAction   (int act)
{
    action=act;
    UpdateControl();
}

void ESceneToolBase::SetSubTarget(int tgt)
{
	sub_target=tgt;
    UpdateControl();
}

void ESceneToolBase::ResetSubTarget()
{
	SetSubTarget(estDefault);
}

