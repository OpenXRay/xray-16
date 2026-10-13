#include "stdafx.h"
#include "OzzMath.h"

namespace XRay::Animation
{
ozz::math::Float4x4 ImportOzzMatrix(const Fmatrix& matrix)
{
    using namespace ozz::math;
    return {{simd_float4::LoadPtrU(&matrix._11), simd_float4::LoadPtrU(&matrix._21),
        simd_float4::LoadPtrU(&matrix._31), simd_float4::LoadPtrU(&matrix._41)}};
}

void ExportOzzMatrix(const ozz::math::Float4x4& matrix, Fmatrix& result)
{
    using namespace ozz::math;
    StorePtrU(matrix.cols[0], &result._11);
    StorePtrU(matrix.cols[1], &result._21);
    StorePtrU(matrix.cols[2], &result._31);
    StorePtrU(matrix.cols[3], &result._41);
}
}
