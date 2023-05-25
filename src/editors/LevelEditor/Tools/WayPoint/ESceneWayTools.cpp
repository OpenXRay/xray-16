#include "stdafx.h"

CCustomObject *ESceneWayTool::CreateObject(LPVOID data, LPCSTR name)
{
    CCustomObject *O = xr_new<CWayObject>(data, name);
    O->FParentTools = this;
    return O;
}
