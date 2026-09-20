// IRenderBackend.cpp
// Virtual destructor definition for ENGINE_API class
#include "stdafx.h"
#include "IRenderBackend.h"

// Default destructor implementation (required for ENGINE_API virtual class)
IRenderBackend::~IRenderBackend() = default;

bool IRenderBackend::RetainBindlessTextures(const u32*, u32)
{
    return false;
}

void IRenderBackend::ReleaseBindlessTextures(const u32*, u32)
{
}
