#include "stdafx.h"
#pragma hdrstop

#include "Layers/xrRender/SH_Atomic.h"
#include "Layers/xrRenderDX11/ResourceManager.h"
#include "Layers/xrRenderDX11/dx11VertexLayoutCache.h"

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
    auto* p = static_cast<ID3DState*>(state);
    _RELEASE(p);
    state = nullptr;
    RImplementation.Resources->_DeleteState(this);
}

SDeclaration::~SDeclaration()
{
    RImplementation.Resources->_DeleteDecl(this);
#if defined(USE_DX11)
    if (auto* data = static_cast<DX11DeclBackendData*>(backend_data))
    {
        for (auto& kv : data->vs_to_layout)
            _RELEASE(kv.second);
        xr_delete(data);
        backend_data = nullptr;
    }
#elif defined(USE_OGL)
#   ifdef XR_PLATFORM_WEB
    ForgetVertexArrays(this);
#   endif
    glDeleteVertexArrays(1, &dcl);
#else
#   error No graphics API selected or enabled!
#endif
}
}
