#pragma once
class XRayUIShader:public IUIShader
{
public:
	XRayUIShader();
	virtual void Copy(IUIShader &_in) ;
	virtual void create(LPCSTR sh, LPCSTR tex = 0) ;
	virtual bool inited();
	virtual void destroy();
};