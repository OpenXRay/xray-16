#pragma once

#include <nvrhi/nvrhi.h>
#include "Include/xrRender/EnvironmentRender.h"

class CEnvironment;

namespace xray::render::fg
{
class FGEnvDescriptorRender : public IEnvDescriptorRender
{
public:
    void OnDeviceCreate(CEnvDescriptor& owner) override;
    void OnDeviceDestroy() override;

    void Copy(IEnvDescriptorRender& _in) override;
};

bool ResolveSkyTextures(CEnvironment& environment, nvrhi::ITexture*& sky0, nvrhi::ITexture*& sky1);

class FGEnvironmentRender : public IEnvironmentRender
{
public:
    void Copy(IEnvironmentRender& _in) override;

    void RenderSky(CEnvironment& env) override {}

    void RenderClouds(CEnvironment& env) override {}

    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void Clear() override;
    void lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB) override;
    const particles_systems::library_interface& particles_systems_library() override;

    void DrawSky(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height);
    void DrawSun(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height);
    void InvalidateShadersAndPipelines();

private:
    void InitSkyResources();
    void InitSunResources();

    nvrhi::IDevice* m_device = nullptr;
    nvrhi::BufferHandle m_skyVertexBuffer;
    nvrhi::BufferHandle m_skyIndexBuffer;
    nvrhi::BufferHandle m_skyConstantBuffer;
    nvrhi::TextureHandle m_skyPlaceholderCube;
    nvrhi::SamplerHandle m_skySampler;
    nvrhi::ShaderHandle m_skyVS;
    nvrhi::ShaderHandle m_skyPS;
    nvrhi::InputLayoutHandle m_skyInputLayout;
    nvrhi::BindingLayoutHandle m_skyBindingLayout;
    nvrhi::GraphicsPipelineHandle m_skyPipeline;
    bool m_skyInitialized = false;

    nvrhi::BufferHandle m_sunVertexBuffer;
    nvrhi::BufferHandle m_sunIndexBuffer;
    nvrhi::TextureHandle m_sunPlaceholderTex;
    nvrhi::ShaderHandle m_sunVS;
    nvrhi::ShaderHandle m_sunPS;
    nvrhi::InputLayoutHandle m_sunInputLayout;
    nvrhi::BindingLayoutHandle m_sunBindingLayout;
    nvrhi::GraphicsPipelineHandle m_sunPipeline;
    bool m_sunInitialized = false;
};
} // namespace xray::render::fg
