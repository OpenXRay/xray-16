#include "stdafx.h"
#pragma hdrstop

#include "EScenePSTools.h"
#include "UI_LevelTools.h"
#include "EScenePSControls.h"
#include "FramePS.h"
#include "EParticlesObject.h"

void EScenePSTool::CreateControls()
{
    inherited::CreateDefaultControls(estDefault);
    AddControl(new TUI_ControlPSAdd(estDefault, etaAdd, this));
    // frame
    pFrame = new TfraPS((TComponent*)0);
}

//----------------------------------------------------

void EScenePSTool::RemoveControls()
{
    inherited::RemoveControls();
}

//----------------------------------------------------

CCustomObject *EScenePSTool::CreateObject(LPVOID data, LPCSTR name)
{
    CCustomObject*O = new EParticlesObject(data, name);
    O->ParentTool = this;
    return O;
}

//----------------------------------------------------
bool EScenePSTool::ExportGame(SExportStreams *F)
{
    return inherited::ExportGame(F);
}

