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
#if !defined(USE_DX9) && !defined(USE_OGL)
//  ,signature(0)
#endif
{}

SVS::~SVS()
{
    RImplementation.Resources->_DeleteVS(this);

#if !defined(USE_DX9) && !defined(USE_OGL)
    // XXX: check just in case
    //_RELEASE(signature);
    //	Now it is release automatically
#endif

#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif
}

///////////////////////////////////////////////////////////////////////
// SPS
SPS::~SPS()
{
#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif
    
    RImplementation.Resources->_DeletePS(this);
}

#ifndef USE_DX9
///////////////////////////////////////////////////////////////////////
// SGS
SGS::~SGS()
{
#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif

    RImplementation.Resources->_DeleteGS(this);
}

#if defined(USE_DX11)
SHS::~SHS()
{
#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif

    RImplementation.Resources->_DeleteHS(this);
}

SDS::~SDS()
{
#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif

    RImplementation.Resources->_DeleteDS(this);
}

SCS::~SCS()
{
#ifdef USE_OGL
    CHK_GL(glDeleteProgram(sh));
#else
    _RELEASE(sh);
#endif

    RImplementation.Resources->_DeleteCS(this);
}
#endif
#endif // !USE_DX9

#if !defined(USE_DX9) && !defined(USE_OGL)
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
#endif // !USE_DX9 && !USE_OGL

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
#elif !defined(USE_DX9)
    xr_map<ID3DBlob*, ID3DInputLayout*>::iterator iLayout;
    iLayout = vs_to_layout.begin();
    for (; iLayout != vs_to_layout.end(); ++iLayout)
    {
        //	Release vertex layout
        _RELEASE(iLayout->second);
    }
#else // USE_DX9
    _RELEASE(dcl);
#endif
}
