#pragma once
class XRayRainRender:public IRainRender
{
public:
	XRayRainRender();
	virtual void Copy(IRainRender &_in) ;

	virtual void Render(CEffect_Rain& owner);

	virtual const Fsphere& GetDropBounds() const;
};
