#include "dxUIShader.h"
#include "stdafx.h"

void dxUIShader::Copy(IUIShader& _in)
{
    *this = *((dxUIShader*)&_in);
}

void dxUIShader::create(LPCSTR sh, LPCSTR tex)
{
    hShader.create(sh, tex);
}

void dxUIShader::destroy()
{
    hShader.destroy();
}