#pragma once

class XRayThunderboltDescRender:public IThunderboltDescRender
{
public:
	XRayThunderboltDescRender();
	virtual void Copy(IThunderboltDescRender &_in) ;

	virtual void CreateModel(LPCSTR m_name) ;
	virtual void DestroyModel() ;
};
