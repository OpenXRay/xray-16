#pragma once

#include <nvrhi/nvrhi.h>
#include "Include/xrRender/EnvironmentRender.h"

class CEnvironment;

namespace xray::render::fg
{
class dxRender_Visual;
class FGEnvironmentRender;

class FGEnvDescriptorRender : public IEnvDescriptorRender
{
    friend class FGEnvironmentRender;

public:
    void OnDeviceCreate(CEnvDescriptor& owner) override;
    void OnDeviceDestroy() override;

    void Copy(IEnvDescriptorRender& _in) override;

private:
    ref_texture sky_texture;
    ref_texture sky_texture_env;
    ref_texture clouds_texture;
};

class FGEnvironmentRender : public IEnvironmentRender
{
public:
    FGEnvironmentRender();

    void Copy(IEnvironmentRender& _in) override;

    void RenderSky(CEnvironment& env) override {}

    void RenderClouds(CEnvironment& env) override {}

    void OnDeviceCreate() override;
    void OnDeviceDestroy() override;
    void Clear() override;
    void lerp(CEnvDescriptorMixer& currentEnv, IEnvDescriptorRender* inA, IEnvDescriptorRender* inB) override;
    const particles_systems::library_interface& particles_systems_library() override;

    void DrawSky(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height, nvrhi::ITexture* depthTex = nullptr, bool composite = false);
    void DrawClouds(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height);
    void DrawPortals(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 width, u32 height);
    void DrawLodImpostors(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, u32 width, u32 height, const xr_vector<dxRender_Visual*>& lods);
    void DrawSun(nvrhi::ICommandList* cmdList, nvrhi::IFramebuffer* framebuffer, CEnvironment* environment, u32 width, u32 height);

private:
    void InitSkyResources();
    void InitSunResources();
    void InitCloudResources();
    void InitPortalResources();
    void InitLodResources();

    using RuntimeTextureList = xr_vector<std::pair<u32, ref_texture>>;
    RuntimeTextureList sky_r_textures;
    RuntimeTextureList clouds_r_textures;

    ref_texture tsky0, tsky1;
    ref_texture t_envmap_0, t_envmap_1;
    ref_texture tonemap;

    u32 tsky0_tstage{};
    u32 tsky1_tstage{};
    u32 tclouds0_tstage{};
    u32 tclouds1_tstage{};
    u32 tonemap_tstage_2sky{ u32(-1) };
    u32 tonemap_tstage_clouds{ u32(-1) };

    nvrhi::IDevice* m_device = nullptr;
    nvrhi::BufferHandle m_skyVertexBuffer;
    nvrhi::BufferHandle m_skyIndexBuffer;
    nvrhi::BufferHandle m_skyConstantBuffer;
    nvrhi::TextureHandle m_skyPlaceholderCube;
    nvrhi::TextureHandle m_skyExposureFallback;
    nvrhi::TextureHandle m_skyDepthFallback;
    nvrhi::BufferHandle m_skyPassCB;
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

    nvrhi::BufferHandle m_cloudVertexBuffer;
    nvrhi::BufferHandle m_cloudIndexBuffer;
    nvrhi::TextureHandle m_cloudPlaceholder;
    nvrhi::SamplerHandle m_cloudSampler;
    nvrhi::ShaderHandle m_cloudVS;
    nvrhi::ShaderHandle m_cloudPS;
    nvrhi::InputLayoutHandle m_cloudInputLayout;
    nvrhi::BindingLayoutHandle m_cloudBindingLayout;
    nvrhi::GraphicsPipelineHandle m_cloudPipeline;
    u32 m_cloudVBCapacity = 0;
    u32 m_cloudIBCapacity = 0;
    bool m_cloudInitialized = false;

    nvrhi::BufferHandle m_portalVertexBuffer;
    nvrhi::ShaderHandle m_portalVS;
    nvrhi::ShaderHandle m_portalPS;
    nvrhi::InputLayoutHandle m_portalInputLayout;
    nvrhi::BindingLayoutHandle m_portalBindingLayout;
    nvrhi::GraphicsPipelineHandle m_portalPipeline;
    u32 m_portalVBCapacity = 0;
    bool m_portalInitialized = false;

    nvrhi::BufferHandle m_lodVertexBuffer;
    nvrhi::BufferHandle m_lodIndexBuffer;
    nvrhi::TextureHandle m_lodPlaceholder;
    nvrhi::SamplerHandle m_lodSampler;
    nvrhi::ShaderHandle m_lodVS;
    nvrhi::ShaderHandle m_lodPS;
    nvrhi::InputLayoutHandle m_lodInputLayout;
    nvrhi::BindingLayoutHandle m_lodBindingLayout;
    nvrhi::GraphicsPipelineHandle m_lodPipeline;
    u32 m_lodVBCapacity = 0;
    bool m_lodInitialized = false;
};
} // namespace xray::render::fg
