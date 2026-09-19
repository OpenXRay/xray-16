#pragma once

#include <thread>
#include <memory>

#include "SkeletonAnimated.h"
#include "xrAnimation/OzzMotionLibrary.h"
#include "xrAnimation/OzzSkeletonMirror.h"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

namespace XRay
{
namespace Animation
{
class OzzKinematicsAnimated : public xray::render::fg::CKinematicsAnimated
{
public:
    OzzKinematicsAnimated();
    ~OzzKinematicsAnimated() override;

    void Load(const char* N, IReader* data, u32 dwFlags) override;
    void Copy(xray::render::fg::dxRender_Visual* P) override;
    void LL_BuldBoneMatrixDequatize(const CBoneData* bd, u8 channel_mask, SKeyTable& keys) override;

    std::shared_ptr<const OzzSkeletonMirror> mirror;

private:
    struct BlendSample
    {
        u32 frame{ u32(-1) };
        float time{ -1.f };
        MotionID motion;
        ozz::animation::SamplingJob::Context ctx;
        xr_vector<CKey> keys;
    };

    void ValidateLibrary(const char* N, const shared_str& key, size_t slot);

    const BlendSample& EnsureSampled(CBlend& B);

    xr_vector<BlendSample> samples;
    ozz::vector<ozz::math::SoaTransform> locals;
    svector<u16, MAX_BLENDED_POOL> live;
#ifdef DEBUG
    std::thread::id sampleOwner;
#endif
    u32 sampleFrame{ u32(-1) };
    xr_vector<OzzMotionLibrary*> libraries;
};
}
}
