#pragma once

#include "xrCore/Animation/SkeletonMotionDefs.hpp"

class CBlend;
typedef svector<CBlend*, MAX_BLENDED * MAX_CHANNELS> BlendSVec; //*MAX_CHANNELS
typedef BlendSVec::iterator BlendSVecIt;
typedef BlendSVec::const_iterator BlendSVecCIt;
