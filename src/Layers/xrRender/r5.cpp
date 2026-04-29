#include "stdafx.h"
#include "r5.h"

#include "xrEngine/IRenderBackend.h"
#include "Layers/xrRender_R2/r2.h"
#include "Layers/xrRender/RenderContext/RenderDevice.h"
#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/FrameGraph/ShaderLoader.h"
#include "Layers/xrRender/Shaders/SlangCompiler.h"
#include "Layers/xrRender/Materials/MaterialSystem.h"
#include "Layers/xrRender/UIRenderCollector.h"

namespace xray::render
{
    void InitializeImGuiRenderer(fg::RenderDevice* renderDevice);
}

void r5::InitializeFrameGraph()
{
    auto& self = xray::render::fg::RImplementation;

    if (!GEnv.Backend || !GEnv.Backend->IsInitialized())
        return;

    self.m_renderDevice = xr_new<xray::render::fg::RenderDevice>();
    if (!self.m_renderDevice->InitializeFromBackend(GEnv.Backend))
    {
        Msg("! RenderDevice initialization failed");
        xr_delete(self.m_renderDevice);
        return;
    }
    Msg("* RenderDevice initialized successfully");

    xray::render::InitializeImGuiRenderer(self.m_renderDevice);

    self.m_framegraphRenderer = xr_new<xray::render::FrameGraphRenderer>();
    if (!self.m_framegraphRenderer->Initialize(self.m_renderDevice))
    {
        Msg("! FrameGraphRenderer initialization failed");
        xr_delete(self.m_framegraphRenderer);
        return;
    }
    Msg("* FrameGraphRenderer initialized successfully");

    GEnv.FrameGraphRenderer = self.m_framegraphRenderer;
    GEnv.UIRender = self.m_framegraphRenderer->GetUICollector();
    self.m_framegraphRenderer->SetEnabled(true);
    Msg("* FrameGraphRenderer enabled");

    self.m_shaderLoader = xr_new<xray::render::framegraph::ShaderLoader>(
        self.m_renderDevice->GetSlangCompiler());
    if (GEnv.Backend->GetAPI() == IRenderBackend::API::Vulkan)
        self.m_shaderLoader->SetTarget(xray::render::SlangCompiler::Target::SPIRV);
    else
        self.m_shaderLoader->SetTarget(xray::render::SlangCompiler::Target::DXIL);
    Msg("* ShaderLoader initialized (target: %s)",
        self.m_shaderLoader->GetTarget() == xray::render::SlangCompiler::Target::SPIRV ? "SPIRV" : "DXIL");

    xray::render::MaterialSystem::Instance().Initialize(
        self.m_renderDevice->GetFGResourceManager(),
        self.m_shaderLoader);
    Msg("* MaterialSystem initialized");
}

void r5::ShutdownFrameGraph()
{
    auto& self = xray::render::fg::RImplementation;

    xray::render::MaterialSystem::Instance().Shutdown();
    Msg("* MaterialSystem shutdown");

    if (self.m_shaderLoader)
    {
        xr_delete(self.m_shaderLoader);
        Msg("* ShaderLoader destroyed");
    }

    if (self.m_framegraphRenderer)
    {
        self.m_framegraphRenderer->Shutdown();
        GEnv.FrameGraphRenderer = nullptr;
        xr_delete(self.m_framegraphRenderer);
        Msg("* FrameGraphRenderer destroyed");
    }

    if (self.m_renderDevice)
    {
        self.m_renderDevice->Shutdown();
        xr_delete(self.m_renderDevice);
        Msg("* RenderDevice destroyed");
    }
}
