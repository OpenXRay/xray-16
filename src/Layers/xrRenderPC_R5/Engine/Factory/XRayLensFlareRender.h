#pragma once
class XRayLensFlareRender:public ILensFlareRender
{
public:
	XRayLensFlareRender();
	virtual void Copy(ILensFlareRender &_in);


	virtual void Render(CLensFlare& owner, BOOL bSun, BOOL bFlares, BOOL bGradient);
	virtual void OnDeviceCreate();
	virtual void OnDeviceDestroy();
};
