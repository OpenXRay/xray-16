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

namespace xray::render::fg
{
using VertexBufferHandle   = nvrhi::BufferHandle;
using IndexBufferHandle    = nvrhi::BufferHandle;
using ConstantBufferHandle = nvrhi::BufferHandle;
using HostBufferHandle     = void*;
}

#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/FVF.h"

#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"

#include "Common/_d3d_extensions.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/xrRender_console.h"

#include "Layers/xrRender/r_FrameGraphRenderer.h"
#include "Layers/xrRender/r2_types.h"

