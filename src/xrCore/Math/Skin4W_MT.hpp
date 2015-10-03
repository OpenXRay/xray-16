#pragma once
#include "MathUtil.hpp"

namespace XRay
{
namespace Math
{

extern Skin4WFunc Skin4W_MTs;

void Skin4W_MT(vertRender *dst, vertBoned4W *src, u32 vCount, CBoneInstance *bones);

} // namespace Math
} // namespace XRay
