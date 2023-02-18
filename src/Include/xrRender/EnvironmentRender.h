#pragma once

namespace particles_systems
{
class library_interface;
} // namespace particle_systems

class CEnvironment;
class CEnvDescriptor;
class CEnvDescriptorMixer;

class XR_NOVTABLE IEnvDescriptorRender
{
public:
    virtual ~IEnvDescriptorRender() = 0;
    virtual void Copy(IEnvDescriptorRender& _in) = 0;

    virtual void OnDeviceCreate(CEnvDescriptor& owner) = 0;
    virtual void OnDeviceDestroy() = 0;
};

inline IEnvDescriptorRender::~IEnvDescriptorRender() = default;

class XR_NOVTABLE IEnvironmentRender
{
public:
    virtual ~IEnvironmentRender() = 0;
    virtual void Copy(IEnvironmentRender& _in) = 0;
    virtual void RenderSky(CEnvironment& env) = 0;
    virtual void RenderClouds(CEnvironment& env) = 0;
    virtual void OnDeviceCreate() = 0;
    virtual void OnDeviceDestroy() = 0;
    virtual void Clear() = 0;
    virtual void lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB) = 0;
    virtual particles_systems::library_interface const& particles_systems_library() = 0;
};

inline IEnvironmentRender::~IEnvironmentRender() = default;
