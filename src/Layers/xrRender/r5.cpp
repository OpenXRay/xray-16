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

    self.m_framegraphRenderer = xr_new<xray::render::FrameGraphRenderer>();
    if (!self.m_framegraphRenderer->Initialize(self.m_renderDevice))
    {
        Msg("! FrameGraphRenderer initialization failed");
        xr_delete(self.m_framegraphRenderer);
        return;
    }
    Msg("* FrameGraphRenderer initialized successfully");

    GEnv.FrameGraphRenderer = self.m_framegraphRenderer;

    xray::render::InitializeImGuiRenderer(self.m_renderDevice);
    GEnv.UIRender = self.m_framegraphRenderer->GetUICollector();
    self.m_framegraphRenderer->SetEnabled(true);
    Msg("* FrameGraphRenderer enabled");

    xray::render::MaterialSystem::Instance().Initialize(
        self.m_renderDevice->GetFGResourceManager(),
        self.m_framegraphRenderer->GetShaderLoader());
    Msg("* MaterialSystem initialized");
}

void r5::ShutdownFrameGraph()
{
    auto& self = xray::render::fg::RImplementation;

    xray::render::MaterialSystem::Instance().Shutdown();
    Msg("* MaterialSystem shutdown");

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
