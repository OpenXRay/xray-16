#pragma once
#include "MathUtil.hpp"

namespace XRay
{
namespace Math
{
void PLCCalc_CPP(int& c0, int& c1, int& c2, const Fvector& camPos, const Fvector* ps, const Fvector& n, const light* l,
    float energy, const Fvector& obj);
} // namespace Math
} // namespace XRay
