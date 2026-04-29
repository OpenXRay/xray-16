#include "stdafx.h"
#pragma hdrstop

#include "SH_Atomic.h"
#include "Layers/xrRenderDX11/ResourceManager.h"

#if defined(USE_DX11)
#include "Layers/xrRender/FrameGraph/ShaderCache.h"  // For ExtractedReflection
#endif

namespace xray::render::fg
{
// Atomic
//SVS::~SVS()
//{
//    _RELEASE(vs);
//    dxRenderDeviceRender::Instance().Resources->_DeleteVS(this);
//}
//SPS::~SPS()
//{
//     _RELEASE(ps);
//     dxRenderDeviceRender::Instance().Resources->_DeletePS(this);
//}
//SState::~SState()
//{
//    _RELEASE(state);
//    dxRenderDeviceRender::Instance().Resources->_DeleteState(this);
//}
//SDeclaration::~SDeclaration()
//{
//    _RELEASE(dcl);
//    dxRenderDeviceRender::Instance().Resources->_DeleteDecl(this);
//}

///////////////////////////////////////////////////////////////////////
//  SVS
SVS::~SVS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteVS(this);
}

///////////////////////////////////////////////////////////////////////
// SPS
SPS::~SPS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeletePS(this);
}

///////////////////////////////////////////////////////////////////////
// SGS
SGS::~SGS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteGS(this);
}

SHS::~SHS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteHS(this);
}

SDS::~SDS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
    if (reflection)
    {
        xr_delete(reflection);
        reflection = nullptr;
    }

    RImplementation.Resources->_DeleteDS(this);
}

SCS::~SCS()
{
    // nvrhiShader is automatically cleaned up by NVRHI refcounting

    // Release extracted reflection data
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
#endif // USE_OGL


#if defined(USE_DX11)
///////////////////////////////////////////////////////////////////////
//	SInputSignature
SInputSignature::SInputSignature(ID3DBlob* pBlob)
{
    VERIFY(pBlob);
    signature = pBlob;
    signature->AddRef();
};

SInputSignature::~SInputSignature()
{
    _RELEASE(signature);
    RImplementation.Resources->_DeleteInputSignature(this);
}
#endif // USE_DX11

///////////////////////////////////////////////////////////////////////
//	SState
SState::~SState()
{
    _RELEASE(state);
    RImplementation.Resources->_DeleteState(this);
}

///////////////////////////////////////////////////////////////////////
//	SDeclaration
SDeclaration::~SDeclaration()
{
    RImplementation.Resources->_DeleteDecl(this);
    //	Release vertex layout
#if defined(USE_DX11)
    xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
    iLayout = vs_to_layout.begin();
    for (; iLayout != vs_to_layout.end(); ++iLayout)
    {
        //	Release vertex layout
        _RELEASE(iLayout->second);
    }
#elif defined(USE_OGL)
    glDeleteVertexArrays(1, &dcl);
#else
#   error No graphics API selected or enabled!
#endif
}
} // namespace xray::render::fg
