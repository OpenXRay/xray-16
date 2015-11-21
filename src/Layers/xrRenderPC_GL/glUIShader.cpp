#include "stdafx.h"
#include "glUIShader.h"

void glUIShader::Copy(IUIShader&_in)
{
	*this = *((glUIShader*)&_in);
}

void glUIShader::create(LPCSTR sh, LPCSTR tex)
{
	hShader.create(sh, tex);
}

void glUIShader::destroy()
{
	hShader.destroy();
}
