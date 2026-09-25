#include "stdafx.h"
#include "IRenderBackend.h"

#include <SDL3/SDL.h>

// Default destructor implementation (required for ENGINE_API virtual class)
IRenderBackend::~IRenderBackend() = default;

IRenderBackend::MemoryBudget IRenderBackend::GetMemoryBudget() const
{
    return {};
}

bool IRenderBackend::RetainBindlessTextures(const u32*, u32)
{
    return false;
}

void IRenderBackend::ReleaseBindlessTextures(const u32*, u32)
{
}

bool IRenderBackend::IsSubmissionLeaseSubmitted(u64) const
{
    return false;
}

nvrhi::ITexture* IRenderBackend::GetBindlessTexture(u32)
{
    return nullptr;
}

bool IRenderBackend::SetHDROutput(bool enabled)
{
    return !enabled;
}

IRenderBackend::DisplayOutput IRenderBackend::GetDisplayOutput() const
{
    return {};
}

bool IRenderBackend::IsWindowHDREnabled(SDL_Window* window)
{
    return window && SDL_GetBooleanProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN, false);
}

IRenderBackend::DisplayOutput IRenderBackend::QueryWindowDisplayOutput(SDL_Window* window, bool hdrActive)
{
    DisplayOutput output;
    output.hdr = hdrActive;
    if (!hdrActive || !window)
        return output;
    const SDL_PropertiesID properties = SDL_GetWindowProperties(window);
    output.sdrWhiteLevel = SDL_GetFloatProperty(properties, SDL_PROP_WINDOW_SDR_WHITE_LEVEL_FLOAT, 1.0f);
    output.headroom = SDL_GetFloatProperty(properties, SDL_PROP_WINDOW_HDR_HEADROOM_FLOAT, 1.0f);
    return output;
}
