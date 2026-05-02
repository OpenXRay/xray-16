#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRenderDX11/ResourceManager.h"

#if defined(USE_DX11)
#include "Layers/xrRender/FrameGraph/ShaderCache.h"
#endif

namespace xray::render::fg
{

SVS::~SVS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteVS(this);
}

SPS::~SPS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeletePS(this);
}

SGS::~SGS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteGS(this);
}

SHS::~SHS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteHS(this);
}

SDS::~SDS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteDS(this);
}

SCS::~SCS()
{

    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteCS(this);
}

#if defined(USE_OGL)
SPP::~SPP()
{
    if (GLAD_GL_ARB_separate_shader_objects)
        CHK_GL(glDeleteProgramPipelines(1, &pp));
    else
        CHK_GL(glDeleteProgram(pp));

    RImplementation.Resources->_DeletePP(this);
}
#endif


SState::~SState()
{
    state = nullptr;
    RImplementation.Resources->_DeleteState(this);
}

SDeclaration::~SDeclaration()
{
    RImplementation.Resources->_DeleteDecl(this);
}
}
