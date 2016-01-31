#include "stdafx.h"
#pragma hdrstop

#include "ESceneAIMapTools.h"
#include "UI_LevelTools.h"
#include "ESceneAIMapControls.h"
#include "FrameAIMap.h"

void ESceneAIMapTool::CreateControls()
{
    inherited::CreateDefaultControls(estDefault);
    // node tools
    //    AddControl(new TUI_ControlAIMapNodeSelect(estAIMapNode,		etaSelect, 	this));
    AddControl(new TUI_ControlAIMapNodeAdd(estAIMapNode, etaAdd, this));
    AddControl(new TUI_ControlAIMapNodeMove(estAIMapNode, etaMove, this));
    AddControl(new TUI_ControlAIMapNodeRotate(estAIMapNode, etaRotate, this));
    // frame
    pFrame = new TfraAIMap((TComponent*)0, this);
}

//----------------------------------------------------

void ESceneAIMapTool::RemoveControls()
{
    inherited::RemoveControls();
}

//----------------------------------------------------


