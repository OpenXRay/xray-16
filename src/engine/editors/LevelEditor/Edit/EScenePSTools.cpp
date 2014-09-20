#include "stdafx.h"
#pragma hdrstop

#include "EScenePSTools.h"
#include "ui_leveltools.h"
#include "EScenePSControls.h"
#include "FramePS.h"
#include "EParticlesObject.h"

void EScenePSTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
    AddControl		(xr_new<TUI_ControlPSAdd>(estDefault,etaAdd,		this));
	// frame
    pFrame 			= xr_new<TfraPS>((TComponent*)0);
}
//----------------------------------------------------

void EScenePSTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

CCustomObject* EScenePSTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<EParticlesObject>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------
bool  EScenePSTool::ExportGame(SExportStreams* F)
{
	return inherited::ExportGame	(F);
}

