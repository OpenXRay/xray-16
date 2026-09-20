#pragma once

#include "xrAnimation.h"
#include "OzzMotionLibrary.h"
#include "Include/xrRender/animation_motion.h"
#include <ozz/base/span.h>

namespace XRay::Animation
{
struct OzzPoseOverride
{
    u16 blend = u16(-1);
    bool isolated = false;
    const float* time = nullptr;
    const Fquaternion* rotation = nullptr;
    const Fvector* translation = nullptr;
};

class XRANIMATION_API OzzPose
{
public:
    OzzPose();
    ~OzzPose();
    OzzPose(const OzzPose&) = delete;
    OzzPose& operator=(const OzzPose&) = delete;

    void Reset(std::shared_ptr<const OzzModelAnimations> assets, u16 blendCapacity);
    void ReserveBlend(u16 slot);
    bool SetBlend(u16 slot, MotionID motion, float time, float weight, u8 channel);
    bool SetBoneBlends(u16 bone, ozz::span<const u16> slots);
    bool SetChannelFactor(u16 channel, float factor);
    const Fmatrix& EvaluateLocalBone(u16 bone, u8 channels);
    void QueryBone(Fmatrix& result, u16 bone, const Fmatrix& parent, u8 channels,
        const OzzPoseOverride& controls);

private:
    struct State;
    std::unique_ptr<State> state;
};
}
