#pragma once

#include <memory>

#include "SkeletonAnimated.h"
#include "xrAnimation/OzzSkeletonMirror.h"

namespace XRay
{
namespace Animation
{
class OzzKinematicsAnimated : public xray::render::fg::CKinematicsAnimated
{
public:
    void Load(const char* N, IReader* data, u32 dwFlags) override;
    void Copy(xray::render::fg::dxRender_Visual* P) override;

    std::shared_ptr<const OzzSkeletonMirror> mirror;
};
}
}
