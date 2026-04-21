#pragma once

#include "xrEngine/Engine.h"

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

class ENGINE_API IFrameGraphRender
{
public:
    virtual ~IFrameGraphRender();

    virtual void Render() = 0;

    virtual void RenderMenu() = 0;

    virtual void RenderStatsOverlay() = 0;

    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;

    virtual xray::render::fg::RenderDevice* GetRenderDevice() const = 0;
    virtual xray::render::MaterialCache* GetMaterialCache() const = 0;
    virtual xray::render::MaterialCache* GetUIMaterialCache() const = 0;
    virtual xray::render::ui::UIRenderCollector* GetUICollector() const = 0;
    virtual xray::render::ui::NVRHIUIRenderer* GetUIRenderer() const = 0;
    virtual xray::render::MaterialCache* GetTextMaterialCache() const = 0;

    virtual void UpdateSmokeTrail(const Fvector& muzzlePos, const Fvector& muzzleDir, float dt, bool isHUDMode) = 0;

    virtual void NotifySmokeShot() = 0;
};
