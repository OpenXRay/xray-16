#pragma once

#include "Common/Common.hpp"

#include "xrEngine/stdafx.h"

#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "xrEngine/IGame_Level.h"

#include "xrParticles/psystem.h"

#pragma warning(push, 0)
#include <nvrhi/nvrhi.h>
#pragma warning(pop)

// Tracy D3D11 replaced by custom profiler (macros in xrCore/Profiler/Profiler.h)

#define R_GL 0
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_R4

#include "Layers/xrRender/VertexLayout.h"

#include "Layers/xrRenderDX11/dx11HW.h"

#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FVF.h"

#include "Layers/xrRenderDX11/Blender.h"
#include "Layers/xrRenderDX11/Blender_CLSID.h"

#include "Common/_d3d_extensions.h"

#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRender/xrRender_console.h"

#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r4_rendertarget.h"

namespace xray::render::fg
{
IC void jitter(CBlender_Compile& C)
{
    //	C.r_Sampler	("jitter0",	JITTER(0), true, nvrhi::SamplerAddressMode::Wrap, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    //	C.r_Sampler	("jitter1",	JITTER(1), true, nvrhi::SamplerAddressMode::Wrap, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    //	C.r_Sampler	("jitter2",	JITTER(2), true, nvrhi::SamplerAddressMode::Wrap, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    //	C.r_Sampler	("jitter3",	JITTER(3), true, nvrhi::SamplerAddressMode::Wrap, SamplerFilter::Point, SamplerFilter::Point, SamplerFilter::Point);
    C.r_dx11Texture("jitter0", JITTER(0));
    C.r_dx11Texture("jitter1", JITTER(1));
    C.r_dx11Texture("jitter2", JITTER(2));
    C.r_dx11Texture("jitter3", JITTER(3));
    C.r_dx11Texture("jitter4", JITTER(4));
    C.r_dx11Texture("jitterMipped", r2_jitter_mipped);
    C.r_dx11Sampler("smp_jitter");
}
} // namespace xray::render::fg
