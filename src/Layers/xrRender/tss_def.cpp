#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/tss_def.h"

namespace xray::render::fg
{
void SimulatorStates::SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode u, nvrhi::SamplerAddressMode v, nvrhi::SamplerAddressMode w)
{
    samplerUsed[slot] = true;
    samplers[slot].addressU = u;
    samplers[slot].addressV = v;
    samplers[slot].addressW = w;
}

void SimulatorStates::SetSamplerAddress(u32 slot, nvrhi::SamplerAddressMode mode)
{
    SetSamplerAddress(slot, mode, mode, mode);
}

void SimulatorStates::SetSamplerAddressU(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressU = mode;
}

void SimulatorStates::SetSamplerAddressV(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressV = mode;
}

void SimulatorStates::SetSamplerAddressW(u32 slot, nvrhi::SamplerAddressMode mode)
{
    samplerUsed[slot] = true;
    samplers[slot].addressW = mode;
}

void SimulatorStates::SetSamplerFilter(u32 slot, bool _min, bool _mip, bool _mag)
{
    samplerUsed[slot] = true;
    samplers[slot].minFilter = _min;
    samplers[slot].mipFilter = _mip;
    samplers[slot].magFilter = _mag;
}

void SimulatorStates::SetSamplerFilterMin(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].minFilter = linear;
}

void SimulatorStates::SetSamplerFilterMip(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].mipFilter = linear;
}

void SimulatorStates::SetSamplerFilterMag(u32 slot, bool linear)
{
    samplerUsed[slot] = true;
    samplers[slot].magFilter = linear;
}

void SimulatorStates::SetSamplerAnisotropic(u32 slot, u32 level)
{
    samplerUsed[slot] = true;
    samplers[slot].maxAnisotropy = static_cast<float>(level);
}

void SimulatorStates::SetSamplerComparison(u32 slot, bool enable)
{
    samplerUsed[slot] = true;
    samplers[slot].reductionType = enable ? nvrhi::SamplerReductionType::Comparison : nvrhi::SamplerReductionType::Standard;
}

void SimulatorStates::SetSamplerBorderColor(u32 slot, u32 packed_argb)
{
    samplerUsed[slot] = true;
    samplers[slot].borderColor.r = ((packed_argb >> 16) & 0xff) / 255.0f;
    samplers[slot].borderColor.g = ((packed_argb >> 8)  & 0xff) / 255.0f;
    samplers[slot].borderColor.b = ((packed_argb)       & 0xff) / 255.0f;
    samplers[slot].borderColor.a = ((packed_argb >> 24) & 0xff) / 255.0f;
}

void SimulatorStates::SetSamplerMipLODBias(u32 slot, float bias)
{
    samplerUsed[slot] = true;
    samplers[slot].mipBias = bias;
}

void SimulatorStates::record(void*& state) { state = nullptr; }

void SimulatorStates::clear() { *this = SimulatorStates{}; }

BOOL SimulatorStates::equal(const SimulatorStates& other) const
{
    return std::memcmp(this, &other, sizeof(*this)) == 0;
}
}
