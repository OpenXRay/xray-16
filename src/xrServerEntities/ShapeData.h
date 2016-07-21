#pragma once
#ifndef ShapeDataH
#define ShapeDataH
#include "xrCore/_types.h"
#include "xrCore/_sphere.h"
#include "xrCore/_matrix.h"
#include "xrCommon/xr_vector.h"

struct CShapeData
{
    enum
    {
        cfSphere = 0,
        cfBox
    };
    union shape_data
    {
        Fsphere sphere;
        Fmatrix box;
    };
    struct shape_def
    {
        u8 type;
        shape_data data;
    };
    DEFINE_VECTOR(shape_def, ShapeVec, ShapeIt);
    ShapeVec shapes;
};

#endif
