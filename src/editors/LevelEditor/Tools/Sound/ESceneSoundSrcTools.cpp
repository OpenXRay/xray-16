#include "stdafx.h"

void ESceneSoundSrcTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
}

void ESceneSoundSrcTool::RemoveControls()
{
	inherited::RemoveControls();
}

CCustomObject *ESceneSoundSrcTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject *O = xr_new<ESoundSource>(data, name);
	O->FParentTools = this;
	return O;
}
