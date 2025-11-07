#pragma once

#include "xrEngine/Engine.h"

class ENGINE_API IFrameGraphRender
{
public:
    virtual ~IFrameGraphRender();

    virtual void Render() = 0;

    virtual void RenderMenu() = 0;

    virtual void SetEnabled(bool enabled) = 0;
    virtual bool IsEnabled() const = 0;
};
