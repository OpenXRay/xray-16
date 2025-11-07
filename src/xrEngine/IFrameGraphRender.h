#pragma once

#include "xrEngine/Engine.h"

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
};
