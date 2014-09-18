#include "stdafx.h"
#pragma hdrstop

#include "ESceneSoundEnvTools.h"
#include "SoundManager_LE.h"
#include "UI_LevelTools.h"
#include "ESound_Environment.h"

void ESceneSoundEnvTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
}
//----------------------------------------------------

void ESceneSoundEnvTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

void ESceneSoundEnvTool::Clear(bool bSpecific)
{
	inherited::Clear	(bSpecific);
    LSndLib->RefreshEnvGeometry	();
}
//----------------------------------------------------

CCustomObject* ESceneSoundEnvTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<ESoundEnvironment>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------

