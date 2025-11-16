#pragma once

#include "xrEngine/Engine.h"

// Forward declarations
namespace xray::render {
    namespace ng {
        class RenderDevice;
    }
    class MaterialCache;
    namespace ui {
        class UIRenderCollector;
        class NVRHIUIRenderer;
    }
}

// ═══════════════════════════════════════════════════════
//  IFRAMEGRAPHRENDER INTERFACE
// ═══════════════════════════════════════════════════════
// Modern FrameGraph-based renderer interface
// Exposed through GEnv.FrameGraphRenderer

class ENGINE_API IFrameGraphRender
{
public:
    virtual ~IFrameGraphRender();

    // Main game rendering (3D world, lighting, post-process)
    virtual void Render() = 0;

    // Main menu rendering (simplified: background + ImGui UI only)
    virtual void RenderMenu() = 0;

    // Enable/disable FrameGraph renderer
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;

    // Accessors for lambda passes to access shared infrastructure
    virtual xray::render::ng::RenderDevice* GetRenderDevice() const = 0;
    virtual xray::render::MaterialCache* GetMaterialCache() const = 0;
    virtual xray::render::MaterialCache* GetUIMaterialCache() const = 0;
    virtual xray::render::ui::UIRenderCollector* GetUICollector() const = 0;
    virtual xray::render::ui::NVRHIUIRenderer* GetUIRenderer() const = 0;
    virtual xray::render::MaterialCache* GetTextMaterialCache() const = 0;
};
