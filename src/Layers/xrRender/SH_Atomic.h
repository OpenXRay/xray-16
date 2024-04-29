#ifndef sh_atomicH
#define sh_atomicH
#pragma once

#include "xrCore/xr_resource.h"
#include "tss_def.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#elif defined(USE_OGL)
#include "Layers/xrRenderGL/glState.h"
#endif

#pragma pack(push, 4)

//////////////////////////////////////////////////////////////////////////
// Atomic resources
//////////////////////////////////////////////////////////////////////////
#if defined(USE_DX11)
struct ECORE_API SInputSignature : public xr_resource_flagged
{
    ID3DBlob* signature;
    SInputSignature(ID3DBlob* pBlob);
    ~SInputSignature();
};
typedef resptr_core<SInputSignature, resptr_base<SInputSignature>> ref_input_sign;
#endif // USE_DX11
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SVS : public xr_resource_named
{
#if defined(USE_DX11)
    ID3DVertexShader* sh;
#elif defined(USE_OGL)
    GLuint sh;
#else
#   error No graphics API selected or enabled!
#endif
    R_constant_table constants;
#if defined(USE_DX11)
    ref_input_sign signature;
#endif
    SVS();
    ~SVS();
};
typedef resptr_core<SVS, resptr_base<SVS>> ref_vs;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SPS : public xr_resource_named
{
#if defined(USE_DX11)
    ID3DPixelShader* sh;
#elif defined(USE_OGL)
    GLuint sh;
#else
#   error No graphics API selected or enabled!
#endif
    R_constant_table constants;
    ~SPS();
};
typedef resptr_core<SPS, resptr_base<SPS>> ref_ps;

#if defined(USE_DX11) || defined(USE_OGL)
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SGS : public xr_resource_named
{
#   if defined(USE_DX11)
    ID3DGeometryShader* sh;
#   elif defined(USE_OGL)
    GLuint sh;
#   else
#       error No graphics API selected or enabled!
#   endif
    R_constant_table constants;
    ~SGS();
};
typedef resptr_core<SGS, resptr_base<SGS>> ref_gs;

struct ECORE_API SHS : public xr_resource_named
{
#   if defined(USE_DX11)
	ID3D11HullShader* sh;
#   elif defined(USE_OGL)
    GLuint sh;
#   else
#       error No graphics API selected or enabled!
#endif
    R_constant_table constants;
    ~SHS();
};
typedef resptr_core<SHS, resptr_base<SHS>> ref_hs;

struct ECORE_API SDS : public xr_resource_named
{
#   if defined(USE_DX11)
    ID3D11DomainShader* sh;
#   elif defined(USE_OGL)
    GLuint sh;
#   else
#       error No graphics API selected or enabled!
#endif
    R_constant_table constants;
    ~SDS();
};
typedef resptr_core<SDS, resptr_base<SDS>> ref_ds;

struct ECORE_API SCS : public xr_resource_named
{
#   if defined(USE_DX11)
    ID3D11ComputeShader* sh;
#   elif defined(USE_OGL)
    GLuint sh;
#   else
#       error No graphics API selected or enabled!
#endif
    R_constant_table constants;
    ~SCS();
};
typedef resptr_core<SCS, resptr_base<SCS>> ref_cs;

#endif // USE_DX11 || USE_OGL

#if defined(USE_OGL)
struct ECORE_API SPP : public xr_resource_named
{
    // Program pipeline object
    // or shader program if ARB_separate_shader_objects is unavailabe
    GLuint pp{};
    R_constant_table constants;

    SPP() = default;
    SPP(GLuint _pp) : pp(_pp) {}
    ~SPP();
};
typedef resptr_core<SPP, resptr_base<SPP>> ref_pp;
#endif // USE_OGL

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
#if defined(USE_DX11)
    //	Maps input signature to input layout
    xr_map<ID3DBlob*, ID3DInputLayout*> vs_to_layout;
    xr_vector<D3D_INPUT_ELEMENT_DESC> dx11_dcl_code;
#elif defined(USE_OGL)
    GLuint dcl;
#else
#   error No graphics API selected or enabled!
#endif

    //	Use this for DirectX10 to cache DX9 declaration for comparison purpose only
    xr_vector<VertexElement> dcl_code;
    SDeclaration() = default;
    ~SDeclaration();
};
typedef resptr_core<SDeclaration, resptr_base<SDeclaration>> ref_declaration;

#pragma pack(pop)
#endif // sh_atomicH
