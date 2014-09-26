#include "stdafx.h"
#pragma hdrstop

#include "ESceneSoundSrcTools.h"
#include "UI_LevelTools.h"
#include "ESound_Source.h"

void ESceneSoundSrcTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
}
//----------------------------------------------------

void ESceneSoundSrcTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

CCustomObject* ESceneSoundSrcTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<ESoundSource>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------

