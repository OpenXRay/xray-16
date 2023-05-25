#include "stdafx.h"

void ESceneAIMapTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
	// node tools
	//    AddControl(xr_new<TUI_ControlAIMapNodeSelect>	(estAIMapNode,		etaSelect, 	this));
	AddControl(xr_new<TUI_ControlAIMapNodeAdd>(estAIMapNode, etaAdd, this));
	AddControl(xr_new<TUI_ControlAIMapNodeMove>(estAIMapNode, etaMove, this));
	AddControl(xr_new<TUI_ControlAIMapNodeRotate>(estAIMapNode, etaRotate, this));
	// frame
	pForm = xr_new<UIAIMapTool>();
	((UIAIMapTool *)pForm)->tool = this;
}

void ESceneAIMapTool::RemoveControls()
{
	inherited::RemoveControls();
}
