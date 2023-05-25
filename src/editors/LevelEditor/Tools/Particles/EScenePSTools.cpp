#include "stdafx.h"

void EScenePSTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
	AddControl(xr_new<TUI_ControlPSAdd>(estDefault, etaAdd, this));
	// frame
	pForm = xr_new<UIParticlesTool>();
}

void EScenePSTool::RemoveControls()
{
	inherited::RemoveControls();
}

CCustomObject *EScenePSTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject *O = xr_new<EParticlesObject>(data, name);
	O->FParentTools = this;
	return O;
}
//----------------------------------------------------
bool EScenePSTool::ExportGame(SExportStreams *F)
{
	return inherited::ExportGame(F);
}
