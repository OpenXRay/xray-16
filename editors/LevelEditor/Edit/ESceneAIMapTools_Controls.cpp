#include "stdafx.h"
#pragma hdrstop

#include "ESceneAIMapTools.h"
#include "ui_leveltools.h"
#include "ESceneAIMapControls.h"
#include "FrameAIMap.h"

void ESceneAIMapTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
	// node tools
//    AddControl(xr_new<TUI_ControlAIMapNodeSelect>	(estAIMapNode,		etaSelect, 	this));
    AddControl(xr_new<TUI_ControlAIMapNodeAdd>		(estAIMapNode,		etaAdd, 	this));
    AddControl(xr_new<TUI_ControlAIMapNodeMove>		(estAIMapNode,		etaMove,	this));
    AddControl(xr_new<TUI_ControlAIMapNodeRotate>	(estAIMapNode,		etaRotate,	this));
	// frame
    pFrame 			= xr_new<TfraAIMap>((TComponent*)0,this);
}
//----------------------------------------------------

void ESceneAIMapTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

