#pragma once

#include "xrCore/xrCore.h"

namespace ozz::animation
{
class Skeleton;
class Animation;
}

#if defined(RENDER_NAMESPACE)
namespace xray::render::RENDER_NAMESPACE
{
class CBackend;
}
#endif

namespace XRay::Animation
{
class OzzKinematicsAnimated;
}

namespace AnimationECS
{
struct AnimationState;
struct AnimationController;
struct AnimationBuffers;
struct AnimationGPUComputeState;
}

namespace XRay::Animation::GPUComputeBridge
{
// Called every frame before submissions begin.
void BeginFrame(float delta_time);

// Submit a character for GPU processing. The ECS runtime remains responsible
// for producing CPU results; the bridge will attempt to mirror state to the
// GPU path for experimentation.
void SubmitCharacter(AnimationECS::AnimationState& state,
                     AnimationECS::AnimationController& controller,
                     AnimationECS::AnimationBuffers& buffers,
                     AnimationECS::AnimationGPUComputeState& gpu_state);

// Render back-end dispatch (called from render thread with command list).
#if defined(RENDER_NAMESPACE)
void DispatchFrame(xray::render::RENDER_NAMESPACE::CBackend& cmd_list,
                   float delta_time);
#endif

// Device lifecycle hooks (invoked by renderer).
void InitializeDevice();
void ShutdownDevice();
}

#if !defined(RENDER_NAMESPACE) && !defined(XR_GPU_ANIMATION_BRIDGE_HAS_IMPLEMENTATION)
namespace XRay::Animation::GPUComputeBridge
{
inline void BeginFrame(float) {}

inline void SubmitCharacter(AnimationECS::AnimationState& /*state*/,
                             AnimationECS::AnimationController& /*controller*/,
                             AnimationECS::AnimationBuffers& /*buffers*/,
                             AnimationECS::AnimationGPUComputeState& gpu_state)
{
    gpu_state.dirty = false;
    gpu_state.gpu_character_idx = u32(-1);
}

inline void InitializeDevice() {}
inline void ShutdownDevice() {}
} // namespace XRay::Animation::GPUComputeBridge
#endif
