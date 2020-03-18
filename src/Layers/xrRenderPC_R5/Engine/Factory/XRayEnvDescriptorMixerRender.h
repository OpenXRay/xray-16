#pragma once
class XRayEnvDescriptorMixerRender :public  IEnvDescriptorMixerRender
{
public:
	XRayEnvDescriptorMixerRender();
	virtual void Copy(IEnvDescriptorMixerRender &_in);

	virtual void Destroy();
	virtual void Clear();
    virtual void lerp(IEnvDescriptorRender* inA, IEnvDescriptorRender* inB);
};
