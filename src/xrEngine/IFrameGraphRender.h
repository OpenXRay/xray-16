#pragma once

#include "xrEngine/Engine.h"

// Forward declarations
namespace xray::render {
    namespace fg {
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

    // Render stats overlay (call between ImGui::NewFrame and EndFrame)
    virtual void RenderStatsOverlay() = 0;

    // Enable/disable FrameGraph renderer
    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;

    // Accessors for lambda passes to access shared infrastructure
    virtual xray::render::fg::RenderDevice* GetRenderDevice() const = 0;
    virtual xray::render::MaterialCache* GetMaterialCache() const = 0;
    virtual xray::render::MaterialCache* GetUIMaterialCache() const = 0;
    virtual xray::render::ui::UIRenderCollector* GetUICollector() const = 0;
    virtual xray::render::ui::NVRHIUIRenderer* GetUIRenderer() const = 0;
    virtual xray::render::MaterialCache* GetTextMaterialCache() const = 0;

    // Weapon smoke trail: feed muzzle transform each frame
    virtual void UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode) = 0;

    // Notify renderer that weapon fired (for smoke heat accumulation)
    virtual void NotifySmokeShot() = 0;
};
