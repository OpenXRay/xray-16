#include "stdafx.h"

void EDetailManager::CreateControls()
{
	AddControl(xr_new<TUI_CustomControl>(estDefault, etaSelect, this));
	// frame
	pForm = xr_new<UIDOTool>();
	((UIDOTool *)pForm)->DM = this;
}
//----------------------------------------------------

void EDetailManager::RemoveControls()
{
	inherited::RemoveControls();
}
