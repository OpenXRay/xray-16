#pragma once

#include "Layers/xrRender/tss_def.h"

namespace xray::render::fg
{
class dx11State;

void UpdateStateDX11(const SimulatorStates& s, dx11State& state);
void UpdateDescDX11(const SimulatorStates& s, D3D_RASTERIZER_DESC& desc);
void UpdateDescDX11(const SimulatorStates& s, D3D_DEPTH_STENCIL_DESC& desc);
void UpdateDescDX11(const SimulatorStates& s, D3D_BLEND_DESC& desc);
void UpdateDescDX11(const SimulatorStates& s, D3D_SAMPLER_DESC descArray[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
    bool SamplerUsed[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT], int iBaseSamplerIndex);
}
