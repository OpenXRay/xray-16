#pragma once

#include "Common/Common.hpp"

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#include "xrEngine/stdafx.h"
#include <d3dx9.h>

#include <D3D11.h>
#include <D3Dx11core.h>
#include <D3DCompiler.h>

#if __has_include(<dxgi1_4.h>)
#include <dxgi1_4.h>
#define HAS_DXGI1_4
#endif

#if __has_include(<d3d11_2.h>)
#include <d3d11_2.h>
#define HAS_DX11_2
#endif

#if __has_include(<d3d11_3.h>)
#include <d3d11_3.h>
#define HAS_DX11_3
#endif

#include "Layers/xrRenderDX11/CommonTypes.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#define R_GL 0
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_R4

#include "xrParticles/psystem.h"
#include "Layers/xrRenderDX10/dx10HW.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"
#include "Layers/xrRender/ResourceManager.h"
#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "Common/_d3d_extensions.h"
#include "xrEngine/IGame_Level.h"
#include "Layers/xrRender/blenders/Blender.h"
#include "Layers/xrRender/blenders/Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "r4.h"

IC void jitter(CBlender_Compile& C)
{
    //	C.r_Sampler	("jitter0",	JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    //	C.r_Sampler	("jitter1",	JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    //	C.r_Sampler	("jitter2",	JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    //	C.r_Sampler	("jitter3",	JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_dx10Texture("jitter0", JITTER(0));
    C.r_dx10Texture("jitter1", JITTER(1));
    C.r_dx10Texture("jitter2", JITTER(2));
    C.r_dx10Texture("jitter3", JITTER(3));
    C.r_dx10Texture("jitter4", JITTER(4));
    C.r_dx10Texture("jitterMipped", r2_jitter_mipped);
    C.r_dx10Sampler("smp_jitter");
}
