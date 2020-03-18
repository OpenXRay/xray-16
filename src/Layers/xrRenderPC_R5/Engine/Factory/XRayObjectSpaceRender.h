#pragma once

class XRayObjectSpaceRender:public IObjectSpaceRender
{
public:
	XRayObjectSpaceRender();
	virtual void Copy(IObjectSpaceRender &_in);;

	virtual void dbgRender();;
	virtual void dbgAddSphere(const Fsphere &sphere, u32 colour);;
	virtual void SetShader();;
};
