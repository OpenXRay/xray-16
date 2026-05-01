#pragma once

#include "xrCore/xr_resource.h"
#include "tss_def.h"

#if defined(USE_DX11)
#include "Layers/xrRenderDX11/StateManager/dx11State.h"
#elif defined(USE_OGL)
#include "Layers/xrRenderGL/glState.h"
#endif

// Forward declarations for Slang reflection
namespace slang {
    struct ShaderReflection;
    struct IComponentType;
}

// Forward declaration for extracted reflection
namespace xray::render::framegraph {
    struct ExtractedReflection;
}

namespace xray::render::fg
{
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
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SVS() = default;
    ~SVS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Vertex; }
};
typedef resptr_core<SVS, resptr_base<SVS>> ref_vs;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SPS : public xr_resource_named
{
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SPS() = default;
    ~SPS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Pixel; }
};
typedef resptr_core<SPS, resptr_base<SPS>> ref_ps;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SGS : public xr_resource_named
{
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SGS() = default;
    ~SGS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Geometry; }
};
typedef resptr_core<SGS, resptr_base<SGS>> ref_gs;

struct ECORE_API SHS : public xr_resource_named
{
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SHS() = default;
    ~SHS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Hull; }
};
typedef resptr_core<SHS, resptr_base<SHS>> ref_hs;

struct ECORE_API SDS : public xr_resource_named
{
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SDS() = default;
    ~SDS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Domain; }
};
typedef resptr_core<SDS, resptr_base<SDS>> ref_ds;

struct ECORE_API SCS : public xr_resource_named
{
    nvrhi::ShaderHandle nvrhiShader;  // NVRHI shader handle
    framegraph::ExtractedReflection* reflection = nullptr;  // Shader reflection metadata
    R_constant_table constants;  // Legacy X-Ray constant table (to be removed in future)

    SCS() = default;
    ~SCS();

    static nvrhi::ShaderType GetShaderType() { return nvrhi::ShaderType::Compute; }
};

struct ECORE_API resptrcode_cs : public resptr_base<SCS>
{
    void create(LPCSTR name);
    void destroy() { _set(nullptr); }
};

typedef resptr_core<SCS, resptrcode_cs> ref_cs;

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
    xr_vector<VertexElement> dcl_code;
#if defined(USE_OGL)
    GLuint dcl;
#endif
    void* backend_data = nullptr;

    SDeclaration() = default;
    ~SDeclaration();
};
typedef resptr_core<SDeclaration, resptr_base<SDeclaration>> ref_declaration;

#pragma pack(pop)
} // namespace xray::render::fg
