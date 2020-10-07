#ifndef sh_atomicH
#define sh_atomicH
#pragma once

#include "xrCore/xr_resource.h"
#include "tss_def.h"

#ifdef USE_OGL
#include "Layers/xrRenderGL/glState.h"
#elif !defined(USE_DX9)
#include "Layers/xrRenderDX10/StateManager/dx10State.h"
#endif

#pragma pack(push, 4)

//////////////////////////////////////////////////////////////////////////
// Atomic resources
//////////////////////////////////////////////////////////////////////////
#if !defined(USE_DX9) && !defined(USE_OGL)
struct ECORE_API SInputSignature : public xr_resource_flagged
{
    ID3DBlob* signature;
    SInputSignature(ID3DBlob* pBlob);
    ~SInputSignature();
};
typedef resptr_core<SInputSignature, resptr_base<SInputSignature>> ref_input_sign;
#endif // !USE_DX9 && !USE_OGL
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SVS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3DVertexShader* sh;
#endif
    R_constant_table constants;
#if !defined(USE_DX9) && !defined(USE_OGL)
    ref_input_sign signature;
#endif
    SVS();
    ~SVS();
};
typedef resptr_core<SVS, resptr_base<SVS>> ref_vs;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SPS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3DPixelShader* sh;
#endif
    R_constant_table constants;
    ~SPS();
};
typedef resptr_core<SPS, resptr_base<SPS>> ref_ps;

#ifndef USE_DX9
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SGS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3DGeometryShader* sh;
#endif
    R_constant_table constants;
    ~SGS();
};
typedef resptr_core<SGS, resptr_base<SGS>> ref_gs;
#endif // !USE_DX9

#if defined(USE_DX11) || defined(USE_OGL)
struct ECORE_API SHS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3D11HullShader* sh;
#endif
    R_constant_table constants;
    ~SHS();
};
typedef resptr_core<SHS, resptr_base<SHS>> ref_hs;

struct ECORE_API SDS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3D11DomainShader* sh;
#endif
    R_constant_table constants;
    ~SDS();
};
typedef resptr_core<SDS, resptr_base<SDS>> ref_ds;

struct ECORE_API SCS : public xr_resource_named
{
#ifdef USE_OGL
    GLuint sh;
#else
    ID3D11ComputeShader* sh;
#endif
    R_constant_table constants;
    ~SCS();
};
typedef resptr_core<SCS, resptr_base<SCS>> ref_cs;

#endif

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SState : public xr_resource_flagged
{
    ID3DState* state;
    SimulatorStates state_code;
    SState() = default;
    ~SState();
};
typedef resptr_core<SState, resptr_base<SState>> ref_state;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SDeclaration : public xr_resource_flagged
{
#if defined(USE_OGL)
    u32 FVF;
    GLuint dcl;
#elif !defined(USE_DX9)
    //	Maps input signature to input layout
    xr_map<ID3DBlob*, ID3DInputLayout*> vs_to_layout;
    xr_vector<D3D_INPUT_ELEMENT_DESC> dx10_dcl_code;
#else // USE_DX9	//	Don't need it: use ID3DInputLayout instead
    //	which is per ( declaration, VS input layout) pair
    IDirect3DVertexDeclaration9* dcl;
#endif // USE_OGL

    //	Use this for DirectX10 to cache DX9 declaration for comparison purpose only
    xr_vector<VertexElement> dcl_code;
    SDeclaration() = default;
    ~SDeclaration();
};
typedef resptr_core<SDeclaration, resptr_base<SDeclaration>> ref_declaration;

#pragma pack(pop)
#endif // sh_atomicH
