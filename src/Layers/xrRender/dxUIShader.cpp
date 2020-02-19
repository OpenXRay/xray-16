#include "stdafx.h"
#include "dxUIShader.h"

void dxUIShader::Copy(IUIShader& _in) { *this = *((dxUIShader*)&_in); }
void dxUIShader::create(const char* sh, const char* tex) { hShader.create(sh, tex); }
void dxUIShader::destroy() { hShader.destroy(); }
