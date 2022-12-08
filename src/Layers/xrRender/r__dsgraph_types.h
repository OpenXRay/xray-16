#pragma once

#include "xrCore/Containers/FixedMap.h"

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

// NORMAL
using mapNormalDirect = xr_vector<_NormalItem>;

struct mapNormalItems : public mapNormalDirect
{
    float ssa;
};

using mapNormal_T = xr_fixed_map<SPass*, mapNormalItems>;
using mapNormalPasses_T = mapNormal_T[SHADER_PASSES_MAX];

// MATRIX
using mapMatrixDirect = xr_vector<_MatrixItem>;

struct mapMatrixItems : public mapMatrixDirect
{
    float ssa;
};

using mapMatrix_T = xr_fixed_map<SPass*, mapMatrixItems>;
using mapMatrixPasses_T = mapMatrix_T[SHADER_PASSES_MAX];

// Top level
using mapSorted_T = xr_fixed_map<float, _MatrixItemS>;
using mapHUD_T    = xr_fixed_map<float, _MatrixItemS>;
using mapLOD_T    = xr_fixed_map<float, _LodItem>;
}
