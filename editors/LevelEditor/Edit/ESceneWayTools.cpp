#include "stdafx.h"
#pragma hdrstop

#include "ESceneWayTools.h"
#include "WayPoint.h"

CCustomObject* ESceneWayTool::CreateObject(LPVOID data, LPCSTR name)
{
	CCustomObject* O	= xr_new<CWayObject>(data,name);
    O->ParentTool		= this;
    return O;
}
//----------------------------------------------------

