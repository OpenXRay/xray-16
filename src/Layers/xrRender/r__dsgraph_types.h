#pragma once

#include "xrCore/fixedmap.h"
#include "Layers/xrRender/RenderAllocator.hpp"

class dxRender_Visual;

//#define USE_RESOURCE_DEBUGGER

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

struct _MatrixItemS : public _MatrixItem
{
    ShaderElement* se;
};

struct _LodItem
{
    float ssa;
    dxRender_Visual* pVisual;
};

#ifdef USE_RESOURCE_DEBUGGER
using vs_type = ref_vs;
using ps_type = ref_ps;
#if defined(USE_DX10) || defined(USE_DX11)
using gs_type = ref_gs;
#ifdef USE_DX11
using hs_type = ref_hs;
using ds_type = ref_ds;
#endif
#endif //	USE_DX10
#else

#if defined(USE_DX10) || defined(USE_DX11) //	DX10 needs shader signature to properly bind geometry to shader
using vs_type = SVS*;
using gs_type = ID3DGeometryShader*;
#ifdef USE_DX11
using hs_type = ID3D11HullShader*;
using ds_type = ID3D11DomainShader*;
#endif
#else //	USE_DX10
using vs_type = ID3DVertexShader* ;
#endif //	USE_DX10
using ps_type = ID3DPixelShader* ;
#endif

// NORMAL
using mapNormalDirect = xr_vector<_NormalItem, render_allocator::helper<_NormalItem>::result>;

struct mapNormalItems : public mapNormalDirect
{
    float ssa;
};

struct mapNormalTextures : public FixedMAP<STextureList*, mapNormalItems, render_allocator>
{
    float ssa;
};

struct mapNormalStates : public FixedMAP<ID3DState*, mapNormalTextures, render_allocator>
{
    float ssa;
};

struct mapNormalCS : public FixedMAP<R_constant_table*, mapNormalStates, render_allocator>
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

struct mapNormalPS : public FixedMAP<ps_type, mapNormalAdvStages, render_allocator>
{
    float ssa;
};
#else
struct mapNormalPS : public FixedMAP<ps_type, mapNormalCS, render_allocator>
{
    float ssa;
};
#endif //	USE_DX11

#if defined(USE_DX10) || defined(USE_DX11)
struct mapNormalGS : public FixedMAP<gs_type, mapNormalPS, render_allocator>
{
    float ssa;
};

struct mapNormalVS : public FixedMAP<vs_type, mapNormalGS, render_allocator> {};
#else //	USE_DX10
struct mapNormalVS : public FixedMAP<vs_type, mapNormalPS, render_allocator> {};
#endif //	USE_DX10

using mapNormal_T = mapNormalVS;
using mapNormalPasses_T = mapNormal_T[SHADER_PASSES_MAX];

// MATRIX
using mapMatrixDirect = xr_vector<_MatrixItem, render_allocator::helper<_MatrixItem>::result>;

struct mapMatrixItems : public mapMatrixDirect
{
    float ssa;
};

struct mapMatrixTextures : public FixedMAP<STextureList*, mapMatrixItems, render_allocator>
{
    float ssa;
};

struct mapMatrixStates : public FixedMAP<ID3DState*, mapMatrixTextures, render_allocator>
{
    float ssa;
};

struct mapMatrixCS : public FixedMAP<R_constant_table*, mapMatrixStates, render_allocator>
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

struct mapMatrixPS : public FixedMAP<ps_type, mapMatrixAdvStages, render_allocator>
{
    float ssa;
};
#else
struct mapMatrixPS : public FixedMAP<ps_type, mapMatrixCS, render_allocator>
{
    float ssa;
};
#endif //	USE_DX11

#if defined(USE_DX10) || defined(USE_DX11)
struct mapMatrixGS : public FixedMAP<gs_type, mapMatrixPS, render_allocator>
{
    float ssa;
};

struct mapMatrixVS : public FixedMAP<vs_type, mapMatrixGS, render_allocator> {};
#else //	USE_DX10
struct mapMatrixVS : public FixedMAP<vs_type, mapMatrixPS, render_allocator> {};
#endif //	USE_DX10

using mapMatrix_T = mapMatrixVS;
using mapMatrixPasses_T = mapMatrix_T[SHADER_PASSES_MAX];

// Top level
using mapSorted_T = FixedMAP<float, _MatrixItemS, render_allocator>;
using mapSorted_Node = mapSorted_T::TNode;

using mapHUD_T = FixedMAP<float, _MatrixItemS, render_allocator>;
using mapHUD_Node = mapHUD_T::TNode;

using mapLOD_T = FixedMAP<float, _LodItem, render_allocator>;
using mapLOD_Node = mapLOD_T::TNode;
}
