// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#ifdef _DEBUG
#      define D3D_DEBUG_INFO
#endif

#pragma warning(disable:4995)
#include "xrEngine/stdafx.h"
#pragma warning(disable:4995)
#include <d3d9.h>
#include <d3dx9.h>
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

#pragma comment( lib, "d3d9.lib" )

#include <d3d10_1.h>
#include <d3d11.h>
#include <D3Dx10core.h>
#include <D3DCompiler.h>

#include "Layers/xrRender/xrD3DDefs.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#define		R_GL	0
#define		R_R1	1
#define		R_R2	2
#define		R_R3	3
#define		R_R4	4
#define		RENDER	R_R3

#include "xrParticles/psystem.h"
#include "Layers/xrRender/HW.h"
#include "Layers/xrRender/Shader.h"
#include "Layers/xrRender/R_Backend.h"
#include "Layers/xrRender/R_Backend_Runtime.h"
#include "Layers/xrRender/ResourceManager.h"
#include "xrEngine/vis_common.h"
#include "xrEngine/Render.h"
#include "Common/_d3d_extensions.h"
#include "xrEngine/IGame_Level.h"
#include "Layers/xrRender/blenders\blender.h"
#include "Layers/xrRender/blenders\blender_clsid.h"
#include "Layers/xrRender/xrRender_console.h"
#include "r3.h"

IC	void	jitter(CBlender_Compile& C)
{
//	C.r_Sampler	("jitter0",	JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
//	C.r_Sampler	("jitter1",	JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
//	C.r_Sampler	("jitter2",	JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
//	C.r_Sampler	("jitter3",	JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
	C.r_dx10Texture	("jitter0",	JITTER(0));
	C.r_dx10Texture	("jitter1",	JITTER(1));
	C.r_dx10Texture	("jitter2",	JITTER(2));
	C.r_dx10Texture	("jitter3",	JITTER(3));
	C.r_dx10Texture	("jitter4",	JITTER(4));
	C.r_dx10Texture	("jitterMipped",	r2_jitter_mipped);
	C.r_dx10Sampler	("smp_jitter");
}
