#pragma once
class XRayEnvDescriptorRender :public IEnvDescriptorRender
{
public:
	XRayEnvDescriptorRender();
	virtual void Copy(IEnvDescriptorRender &_in);

	virtual void OnDeviceCreate(CEnvDescriptor& owner);
	virtual void OnDeviceDestroy();
};
