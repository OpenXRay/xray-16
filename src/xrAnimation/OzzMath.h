#pragma once

#include "xrCore/_matrix.h"
#include <ozz/base/maths/simd_math.h>

namespace XRay::Animation
{
ozz::math::Float4x4 ImportOzzMatrix(const Fmatrix& matrix);
void ExportOzzMatrix(const ozz::math::Float4x4& matrix, Fmatrix& result);
}
