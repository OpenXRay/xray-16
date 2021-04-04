#pragma once

#include "Common/Common.hpp"

#include "xrEngine/stdafx.h"

#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "xrEngine/IGame_Level.h"

#include "xrParticles/psystem.h"

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>
#include <d3dx9.h>

#include "Layers/xrRenderDX9/CommonTypes.h"

#include "Layers/xrRenderDX9/dx9HW.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#include "Layers/xrRender/BufferUtils.h"

#include "Layers/xrRender/Shader.h"

#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"

#include "Layers/xrRender/Blender.h"
#include "Layers/xrRender/Blender_CLSID.h"

#define R_GL 0
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_R1

#include "Common/_d3d_extensions.h"

#include "Layers/xrRender/ResourceManager.h"
#include "Layers/xrRender/xrRender_console.h"

#include "FStaticRender.h"

#define TEX_POINT_ATT "internal" DELIMITER "internal_light_attpoint"
#define TEX_SPOT_ATT "internal" DELIMITER "internal_light_attclip"
