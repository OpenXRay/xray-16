#include "stdafx.h"
#include "ClusterBasis.h"

#include <cmath>

namespace xray::render::fg
{

Fvector3 ClusterNormalizeBasisInput(const Fvector3& n)
{
    const float lenSq = n.x * n.x + n.y * n.y + n.z * n.z;
    if (!(lenSq > 1e-12f) || !std::isfinite(lenSq))
        return Fvector3{ 0.0f, 0.0f, 1.0f };
    const float inv = 1.0f / _sqrt(lenSq);
    return Fvector3{ n.x * inv, n.y * inv, n.z * inv };
}

void ClusterDeriveBasis(const Fvector3& n, Fvector3& outTangent, Fvector3& outBinormal)
{
    const Fvector3 u = ClusterNormalizeBasisInput(n);
    const float s = (u.z >= 0.0f) ? 1.0f : -1.0f;
    const float a = -1.0f / (s + u.z);
    const float b = u.x * u.y * a;
    outTangent.set(1.0f + s * u.x * u.x * a, s * b, -s * u.x);
    outBinormal.set(b, s + u.y * u.y * a, -u.y);
}

}
