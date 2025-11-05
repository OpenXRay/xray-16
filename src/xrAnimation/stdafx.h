#pragma once

// Include xrEngine's stdafx.h like xrGame does
#include "xrEngine/stdafx.h"

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>

// Additional X-Ray Engine includes specific to animation
#include "xrCore/Animation/SkeletonMotions.hpp"
#include "xrCore/Animation/Bone.hpp"
#include "xrCore/FMesh.hpp"
#include "xrCore/xr_ini.h"
#include "xrCore/_math.h"
#include "xrCore/_matrix.h"
#include "xrCore/_quaternion.h"
#include "xrCore/_vector3d.h"

// ozz-animation includes
#include "ozz/animation/offline/raw_skeleton.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/skeleton_builder.h"
#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/maths/transform.h"
#include "ozz/base/maths/quaternion.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/soa_transform.h"
#include "ozz/base/maths/soa_float4x4.h"
