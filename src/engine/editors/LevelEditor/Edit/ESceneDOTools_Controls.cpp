#include "stdafx.h"
#pragma hdrstop

#include "ESceneDOTools.h"
#include "FrameDetObj.h"
#include "UI_LevelTools.h"
#include "ESceneControlsCustom.h"

void EDetailManager::CreateControls()
{
	AddControl		(xr_new<TUI_CustomControl>(estDefault,	etaSelect, this));
	// frame
    pFrame 			= xr_new<TfraDetailObject>((TComponent*)0,this);
}
//----------------------------------------------------
 
void EDetailManager::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

                     