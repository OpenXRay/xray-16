#pragma once
#ifndef ShapeDataH
#define ShapeDataH
#include "xrCore/xr_types.h"
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
    using ShapeVec = xr_vector<shape_def>;
    ShapeVec shapes;
};

#endif
