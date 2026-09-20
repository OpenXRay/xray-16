#include "stdafx.h"
#include "IRenderBackend.h"

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
