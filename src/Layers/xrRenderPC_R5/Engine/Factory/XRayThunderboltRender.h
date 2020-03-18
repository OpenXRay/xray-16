#pragma once
class XRayThunderboltRender:public IThunderboltRender
{
public:
	XRayThunderboltRender();
	virtual void Copy(IThunderboltRender &_in) ;

	virtual void Render(CEffect_Thunderbolt &owner);
};
