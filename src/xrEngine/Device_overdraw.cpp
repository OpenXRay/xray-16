#include "stdafx.h"
#include "Render.h"

void CRenderDevice::overdrawBegin()
{
    VERIFY(0);
    GEnv.Render->overdrawBegin();
}

void CRenderDevice::overdrawEnd()
{
    VERIFY(0);
    GEnv.Render->overdrawEnd();
}
