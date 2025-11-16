#pragma once

#include "xrEngine/Engine.h"

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

class ENGINE_API IFrameGraphRender
{
public:
    virtual ~IFrameGraphRender();

    virtual void Render() = 0;

    virtual void RenderMenu() = 0;

    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;

    virtual xray::render::ng::RenderDevice* GetRenderDevice() const = 0;
    virtual xray::render::MaterialCache* GetMaterialCache() const = 0;
    virtual xray::render::MaterialCache* GetUIMaterialCache() const = 0;
    virtual xray::render::ui::UIRenderCollector* GetUICollector() const = 0;
    virtual xray::render::ui::NVRHIUIRenderer* GetUIRenderer() const = 0;
    virtual xray::render::MaterialCache* GetTextMaterialCache() const = 0;
};
