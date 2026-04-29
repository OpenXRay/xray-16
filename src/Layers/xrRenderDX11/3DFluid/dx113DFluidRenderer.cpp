#include "stdafx.h"
#include "dx113DFluidRenderer.h"
#include "dx113DFluidData.h"

// Legacy 3D Fluid renderer - stubbed out, FrameGraph handles rendering

namespace xray::render::fg
{

LPCSTR dx113DFluidRenderer::m_pRTNames[RRT_NumRT] = {
    "$user$rayDataTex", "$user$rayDataTexSmall", "$user$rayCastTex", "$user$edgeTex"};

LPCSTR dx113DFluidRenderer::m_pResourceRTNames[RRT_NumRT] = {"rayDataTex", "rayDataTexSmall", "rayCastTex", "edgeTex"};

dx113DFluidRenderer::dx113DFluidRenderer() : m_bInited(false) {}
dx113DFluidRenderer::~dx113DFluidRenderer() {}

void dx113DFluidRenderer::Initialize(int, int, int) {}
void dx113DFluidRenderer::Destroy() {}
void dx113DFluidRenderer::SetScreenSize(int, int) {}
void dx113DFluidRenderer::Draw(const dx113DFluidData&) {}

} // namespace xray::render::fg
