#include "stdafx.h"
#include "IRenderBackend.h"

IRenderBackend::~IRenderBackend() = default;

bool IRenderBackend::RetainBindlessTextures(const u32*, u32)
{
    return false;
}

void IRenderBackend::ReleaseBindlessTextures(const u32*, u32)
{
}
