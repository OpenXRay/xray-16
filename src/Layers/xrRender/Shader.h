// Shader.h: interface for the CShader class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(SHADER__INCLUDED_)
#define SHADER__INCLUDED_
#pragma once

#include "r_constants.h"
#include "xrCore/xr_resource.h"

#include "SH_Atomic.h"
#include "SH_Texture.h"
#include "SH_Matrix.h"
#include "SH_Constant.h"
#include "SH_RT.h"

using sh_list = xr_vector<shared_str>;
class CBlender_Compile;
class IBlender;
#define SHADER_PASSES_MAX 2

#pragma pack(push, 4)

//////////////////////////////////////////////////////////////////////////
struct ECORE_API STextureList : public xr_resource_flagged, public xr_vector<std::pair<u32, ref_texture>>
{
    using inherited_vec = xr_vector<std::pair<u32, ref_texture>>;
    ~STextureList();

    BOOL equal(const STextureList& base) const
    {
        if (size() != base.size())
            return FALSE;
        for (u32 cmp = 0; cmp < size(); cmp++)
        {
            if ((*this)[cmp].first != base[cmp].first)
                return FALSE;
            if ((*this)[cmp].second != base[cmp].second)
                return FALSE;
        }
        return TRUE;
    }
    virtual void clear();

    // Avoid using this function.
    // If possible use precompiled texture list.
    u32 find_texture_stage(const shared_str& TexName) const;
};
typedef resptr_core<STextureList, resptr_base<STextureList>> ref_texture_list;
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SMatrixList : public xr_resource_flagged, public svector<ref_matrix, 4>
{
    ~SMatrixList();
};
typedef resptr_core<SMatrixList, resptr_base<SMatrixList>> ref_matrix_list;
//////////////////////////////////////////////////////////////////////////
struct ECORE_API SConstantList : public xr_resource_flagged, public svector<ref_constant_obsolette, 4>
{
    ~SConstantList();
};
typedef resptr_core<SConstantList, resptr_base<SConstantList>> ref_constant_list;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SGeometry : public xr_resource_flagged
{
    ref_declaration dcl;
#ifdef USE_OGL
	GLuint vb;
	GLuint ib;
#else
    ID3DVertexBuffer* vb;
    ID3DIndexBuffer* ib;
#endif
    u32 vb_stride;
    ~SGeometry();
};

struct ECORE_API resptrcode_geom : public resptr_base<SGeometry>
{
#ifdef USE_OGL
    void create(D3DVERTEXELEMENT9* decl, GLuint vb, GLuint ib);
    void create(u32 FVF, GLuint vb, GLuint ib);
#else
    void create(D3DVERTEXELEMENT9* decl, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib);
    void create(u32 FVF, ID3DVertexBuffer* vb, ID3DIndexBuffer* ib);
#endif // USE_OGL
    void destroy() { _set(nullptr); }
    u32 stride() const { return _get()->vb_stride; }
};

typedef resptr_core<SGeometry, resptrcode_geom> ref_geom;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API SPass : public xr_resource_flagged
{
    ref_state state; // Generic state, like Z-Buffering, samplers, etc
    ref_ps ps; // may be NULL = FFP, in that case "state" must contain TSS setup
    ref_vs vs; // may be NULL = FFP, in that case "state" must contain RS setup, *and* FVF-compatible declaration must be used
#ifndef USE_DX9
    ref_gs gs; // may be NULL = don't use geometry shader at all
#ifdef USE_DX11
    ref_hs hs; // may be NULL = don't use hull shader at all
    ref_ds ds; // may be NULL = don't use domain shader at all
    ref_cs cs; // may be NULL = don't use compute shader at all
#endif
#endif // USE_DX10
    ref_ctable constants; // may be NULL

    ref_texture_list T;
    ref_constant_list C;
#ifdef _EDITOR
    ref_matrix_list M;
#endif

    ~SPass();

    BOOL equal(const SPass& other);
};
typedef resptr_core<SPass, resptr_base<SPass>> ref_pass;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API ShaderElement : public xr_resource_flagged
{
    struct Sflags
    {
        u32 iPriority : 2;
        u32 bStrictB2F : 1;
        u32 bEmissive : 1;
        u32 bDistort : 1;
        u32 bWmark : 1;
    };

    Sflags flags;
    svector<ref_pass, SHADER_PASSES_MAX> passes;

    ShaderElement();
    ~ShaderElement();
    BOOL equal(ShaderElement& S);
};
using ref_selement = resptr_core<ShaderElement, resptr_base<ShaderElement>>;

//////////////////////////////////////////////////////////////////////////
struct ECORE_API Shader : public xr_resource_flagged
{
    ref_selement E[6]; // R1 - 0=norm_lod0(det), 1=norm_lod1(normal), 2=L_point, 3=L_spot, 4=L_for_models,
    // R2 - 0=deffer, 1=norm_lod1(normal), 2=psm, 3=ssm, 4=dsm
    ~Shader();
    BOOL equal(Shader* S);
    BOOL equal(Shader* S, int index);
};

struct ECORE_API resptrcode_shader : public resptr_base<Shader>
{
    void create(LPCSTR s_shader = nullptr, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    void create(IBlender* B, LPCSTR s_shader = nullptr, LPCSTR s_textures = nullptr, LPCSTR s_constants = nullptr, LPCSTR s_matrices = nullptr);
    void destroy() { _set(nullptr); }
};

using ref_shader = resptr_core<Shader, resptrcode_shader>;

enum SE_R1
{
    SE_R1_NORMAL_HQ = 0, // high quality/detail
    SE_R1_NORMAL_LQ = 1, // normal or low quality
    SE_R1_LPOINT = 2, // add: point light
    SE_R1_LSPOT = 3, // add: spot light
    SE_R1_LMODELS = 4, // lighting info for models or shadowing from models
};

//#define SE_R2_NORMAL_HQ 0 // high quality/detail
//#define SE_R2_NORMAL_LQ 1 // low quality
//#define SE_R2_SHADOW 2 // shadow generation
//  E[3] - can use for night vision but need to extend SE_R1. Will need
//  Extra shader element.
//  E[4] - distortion or self illumination(self emission).
//  E[4] Can use for lightmap capturing.

#pragma pack(pop)

#endif // !defined(AFX_SHADER_H__9CBD70DD_E147_446B_B4EE_5DA321EB726F__INCLUDED_)
