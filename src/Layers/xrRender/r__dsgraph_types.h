#pragma once

class dxRender_Visual;

namespace R_dsgraph
{
// Elementary types
struct _NormalItem
{
    float ssa;
    dxRender_Visual* pVisual;
};

struct _MatrixItem
{
    float ssa;
    IRenderable* pObject;
    dxRender_Visual* pVisual;
    Fmatrix Matrix; // matrix (copy)
};

struct _MatrixItemS
{
    // Хак для использования списков инициализации
    // Не используем наследование
    // _MatrixItem begin
    float ssa;
    IRenderable* pObject;
    dxRender_Visual* pVisual;
    Fmatrix Matrix; // matrix (copy)
    // _MatrixItem end
    ShaderElement* se;
};

struct _LodItem
{
    float ssa;
    dxRender_Visual* pVisual;
};

using state_type = SState*;
#ifndef USE_OGL
using ps_type = ID3DPixelShader*;
#if defined(USE_DX10) || defined(USE_DX11) // DX10 and DX11 needs shader signature to properly bind geometry to shader
using vs_type = SVS*;
using gs_type = ID3DGeometryShader*;
#else
using vs_type = ID3DVertexShader*;
#endif
#ifdef USE_DX11
using hs_type = ID3D11HullShader*;
using ds_type = ID3D11DomainShader*;
#endif
#else
using vs_type = GLuint;
using ps_type = GLuint;
using gs_type = GLuint;
#endif

// NORMAL
using mapNormalDirect = xr_vector<_NormalItem>;

struct mapNormalItems : public mapNormalDirect
{
    float ssa;
};

struct mapNormalTextures : public xr_unordered_map<STextureList*, mapNormalItems>
{
    float ssa;
};

struct mapNormalStates : public xr_unordered_map<state_type, mapNormalTextures>
{
    float ssa;
};

struct mapNormalCS : public xr_unordered_map<R_constant_table*, mapNormalStates>
{
    float ssa;
};

#ifdef USE_DX11
struct mapNormalAdvStages
{
    hs_type hs;
    ds_type ds;
    mapNormalCS mapCS;
};

struct mapNormalPS : public xr_unordered_map<ps_type, mapNormalAdvStages>
{
    float ssa;
};
#else
struct mapNormalPS : public xr_unordered_map<ps_type, mapNormalCS>
{
    float ssa;
};
#endif

#ifndef USE_DX9
struct mapNormalGS : public xr_unordered_map<gs_type, mapNormalPS>
{
    float ssa;
};

struct mapNormalVS : public xr_unordered_map<vs_type, mapNormalGS> {};
#else
struct mapNormalVS : public xr_unordered_map<vs_type, mapNormalPS> {};
#endif

using mapNormal_T = mapNormalVS;
using mapNormalPasses_T = mapNormal_T[SHADER_PASSES_MAX];

// MATRIX
using mapMatrixDirect = xr_vector<_MatrixItem>;

struct mapMatrixItems : public mapMatrixDirect
{
    float ssa;
};

struct mapMatrixTextures : public xr_unordered_map<STextureList*, mapMatrixItems>
{
    float ssa;
};

struct mapMatrixStates : public xr_unordered_map<state_type, mapMatrixTextures>
{
    float ssa;
};

struct mapMatrixCS : public xr_unordered_map<R_constant_table*, mapMatrixStates>
{
    float ssa;
};

#ifdef USE_DX11
struct mapMatrixAdvStages
{
    hs_type hs;
    ds_type ds;
    mapMatrixCS mapCS;
};

struct mapMatrixPS : public xr_unordered_map<ps_type, mapMatrixAdvStages>
{
    float ssa;
};
#else
struct mapMatrixPS : public xr_unordered_map<ps_type, mapMatrixCS>
{
    float ssa;
};
#endif

#ifndef USE_DX9
struct mapMatrixGS : public xr_unordered_map<gs_type, mapMatrixPS>
{
    float ssa;
};

struct mapMatrixVS : public xr_unordered_map<vs_type, mapMatrixGS> {};
#else
struct mapMatrixVS : public xr_unordered_map<vs_type, mapMatrixPS> {};
#endif

using mapMatrix_T = mapMatrixVS;
using mapMatrixPasses_T = mapMatrix_T[SHADER_PASSES_MAX];

// Top level
using mapSorted_T = xr_vector<std::pair<float, _MatrixItemS>>;
using mapHUD_T    = xr_vector<std::pair<float, _MatrixItemS>>;
using mapLOD_T    = xr_vector<std::pair<float, _LodItem>>;
}
