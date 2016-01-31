#include "stdafx.h"
#pragma hdrstop

#include "ESceneDOTools.h"
#include "FrameDetObj.h"
#include "UI_LevelTools.h"
#include "ESceneControlsCustom.h"

void EDetailManager::CreateControls()
{
    AddControl(new TUI_CustomControl(estDefault, etaSelect, this));
    // frame
    pFrame = new TfraDetailObject((TComponent*)0, this);
}

//----------------------------------------------------

void EDetailManager::RemoveControls()
{
    inherited::RemoveControls();
}

//----------------------------------------------------


