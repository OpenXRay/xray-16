#pragma once

#include "xrCore/xrCore.h"

namespace xray::render::fg
{

Fvector3 ClusterNormalizeBasisInput(const Fvector3& n);

void ClusterDeriveBasis(const Fvector3& n, Fvector3& outTangent, Fvector3& outBinormal);

}
