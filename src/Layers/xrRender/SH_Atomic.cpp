#include "stdafx.h"
#pragma hdrstop

#include "SH_Atomic.h"
#include "ResourceManager.h"

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
SVS::SVS() : sh(0)
#if defined(USE_DX11) || defined(USE_DX12)
//  ,signature(0)
#endif
{}

SVS::~SVS()
{
    RImplementation.Resources->_DeleteVS(this);

#if defined(USE_DX11) || defined(USE_DX12)
    // XXX: check just in case
    //_RELEASE(signature);
    //	Now it is release automatically
#endif

#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#else
#   error No graphics API selected or enabled!
#endif
}

///////////////////////////////////////////////////////////////////////
// SPS
SPS::~SPS()
{
#if defined(USE_DX9) || defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#else
#   error No graphics API selected or enabled!
#endif

    RImplementation.Resources->_DeletePS(this);
}

#if defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
///////////////////////////////////////////////////////////////////////
// SGS
SGS::~SGS()
{
#if defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#   elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#   else
#       error No graphics API selected or enabled!
#   endif

    RImplementation.Resources->_DeleteGS(this);
}

SHS::~SHS()
{
#if defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#   elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#   else
#       error No graphics API selected or enabled!
#   endif

    RImplementation.Resources->_DeleteHS(this);
}

SDS::~SDS()
{
#if defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#   elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#   endif

    RImplementation.Resources->_DeleteDS(this);
}

SCS::~SCS()
{
#if defined(USE_DX11) || defined(USE_DX12)
    _RELEASE(sh);
#    elif defined(USE_OGL)
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgram(sh));
    else
        CHK_GL(glDeleteShader(sh));
#   else
#       error No graphics API selected or enabled!
#   endif

    RImplementation.Resources->_DeleteCS(this);
}
#endif // USE_DX11 || USE_OGL

#if defined(USE_OGL)
SPP::~SPP()
{
    if (HW.SeparateShaderObjectsSupported)
        CHK_GL(glDeleteProgramPipelines(1, &pp));
    else
        CHK_GL(glDeleteProgram(pp));

    RImplementation.Resources->_DeletePP(this);
}
#endif // USE_OGL


#if defined(USE_DX11) || defined(USE_DX12)
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
#ifdef USE_OGL
    glDeleteVertexArrays(1, &dcl);
#elif defined(USE_DX11) || defined(USE_DX12) || defined(USE_OGL)
    xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
    iLayout = vs_to_layout.begin();
    for (; iLayout != vs_to_layout.end(); ++iLayout)
    {
        //	Release vertex layout
        _RELEASE(iLayout->second);
    }
#elif defined(USE_DX9)// USE_DX9
    _RELEASE(dcl);
#else
#   error No graphics API selected or enabled!
#endif
}
