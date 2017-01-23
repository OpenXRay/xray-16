#pragma once

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#pragma warning(disable : 4995)
#include "xrEngine/stdafx.h"
#pragma warning(disable : 4995)
#include <d3d9.h>
#include <d3dx9.h>
#pragma warning(default : 4995)
#pragma warning(disable : 4714)
#pragma warning(4 : 4018)
#pragma warning(4 : 4244)

#pragma comment(lib, "d3d9.lib")

#include "Layers/xrRender/HW.h"
#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/xrD3DDefs.h"

#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_R1

#include "Common/_d3d_extensions.h"
#include "Layers/xrRender/ResourceManager.h"
#include "xrEngine/Render.h"
#include "xrEngine/vis_common.h"
#ifndef _EDITOR
#include "FStaticRender.h"
#include "Layers/xrRender/blenders\Blender.h"
#include "Layers/xrRender/blenders\Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "xrEngine/IGame_Level.h"
#include "xrParticles/psystem.h"
#endif

#define TEX_POINT_ATT "internal\\internal_light_attpoint"
#define TEX_SPOT_ATT "internal\\internal_light_attclip"
