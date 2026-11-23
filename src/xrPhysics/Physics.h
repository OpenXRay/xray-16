#pragma once

#include "PhysicsShell.h"
#include "PHObject.h"
#include "PHInterpolation.h"
#include "xrCore/_cylinder.h"
#include "BlockAllocator.h"
#include "PhysicsCommon.h"
#include "PHWorld.h"
#include "PHContactBodyEffector.h"
#include "phvalide.h"
#include "xrPhysicsCore/IPhysicsCore.h" // Наше ядро Jolt

void BodyCutForce(CharacterVirtualHandle body, float l_limit, float w_limit);
float E_NLD(CharacterVirtualHandle b1, CharacterVirtualHandle b2, const Fvector& norm);
float E_NL( CharacterVirtualHandle b1, CharacterVirtualHandle b2, const Fvector& norm );
float E_NlS(CharacterVirtualHandle body, const Fvector& norm, float norm_sign);

void ApplyGravityAccel(CharacterVirtualHandle body, const Fvector& accel);
const float fix_ext_param = 10000.f;
const float fix_mass_param = 100000000.f;
void FixBody(CharacterVirtualHandle body);

extern class CBlockAllocator<CPHContactBodyEffector, 128> ContactEffectors;

class CPHElement;
class CPHShell;
extern Fbox phBoundaries;

IC bool PhOutOfBoundaries(const Fvector& v) { return v.y < phBoundaries.y1; }
