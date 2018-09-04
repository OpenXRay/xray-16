#pragma once

#pragma warning(disable:4995)
#include "xrEngine/stdafx.h"
#pragma warning(default:4995)
#pragma warning(disable:4714)
#pragma warning( 4 : 4018 )
#pragma warning( 4 : 4244 )
#pragma warning(disable:4237)

#define GLEW_STATIC
#include <GL/glew.h>
#include "SDL_opengl.h"
#include <GL/glu.h>
#include "SDL_opengl_glext.h"

#include "Layers/xrRender/xrD3DDefs.h"
#include "Layers/xrRender/Debug/dxPixEventWrapper.h"

#define R_GL 0
#define R_R1 1
#define R_R2 2
#define R_R3 3
#define R_R4 4
#define RENDER R_GL

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
#include "Layers/xrRender/blenders/Blender.h"
#include "Layers/xrRender/blenders/Blender_CLSID.h"
#include "Layers/xrRender/xrRender_console.h"
#include "rgl.h"

IC void jitter(CBlender_Compile& C)
{
    C.r_Sampler("jitter0", JITTER(0), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter1", JITTER(1), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter2", JITTER(2), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
    C.r_Sampler("jitter3", JITTER(3), true, D3DTADDRESS_WRAP, D3DTEXF_POINT, D3DTEXF_NONE, D3DTEXF_POINT);
}
