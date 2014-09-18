#ifndef EnvironmentRender_included
#define EnvironmentRender_included
#pragma once

namespace particles_systems {
	class library_interface;
} // namespace particle_systems

class CEnvironment;
class CEnvDescriptor;

class IEnvDescriptorRender
{
public:
	virtual ~IEnvDescriptorRender() {;}
	virtual void Copy(IEnvDescriptorRender &_in) = 0;

	virtual void OnDeviceCreate(CEnvDescriptor &owner) = 0;
	virtual void OnDeviceDestroy() = 0;
};

class IEnvDescriptorMixerRender
{
public:
	virtual ~IEnvDescriptorMixerRender() {;}
	virtual void Copy(IEnvDescriptorMixerRender &_in) = 0;

	virtual void Destroy() = 0;
	virtual void Clear() = 0;
	virtual void lerp(IEnvDescriptorRender *inA, IEnvDescriptorRender *inB) = 0;
};

class IEnvironmentRender
{
public:
	virtual ~IEnvironmentRender() {;}
	virtual void Copy(IEnvironmentRender &_in) = 0;
	virtual void OnFrame(CEnvironment &env) = 0;
	virtual void OnLoad() = 0;
	virtual void OnUnload() = 0;
	virtual void RenderSky(CEnvironment &env) = 0;
	virtual void RenderClouds(CEnvironment &env) = 0;
	virtual void OnDeviceCreate() = 0;
	virtual void OnDeviceDestroy() = 0;
	virtual particles_systems::library_interface const& particles_systems_library	() = 0;
};

#endif	//	EnvironmentRender_included