#pragma once

#include "FHierrarhyVisual.h"

#include "xrAnimation/OzzKinematics.h"
#include "xrAnimation/OzzKinematicsAnimated.h"
#include "xrAnimation/OzzBundle.h"
#include "xrCore/_fbox.h"
#include "xrCore/_sphere.h"
#include "xrCore/xrDebug.h"

#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_smart_pointers.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/span.h"
#include "framework/mesh.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace xray::render::RENDER_NAMESPACE
{
using XRay::Animation::OzzKinematics;
using XRay::Animation::OzzKinematicsAnimated;
class COzzSkinnedSurface;

/**
 * Visual that uses the new Ozz-based kinematics system.
 * Holds either OzzKinematics (static) or OzzKinematicsAnimated (animated)
 * based on whether animation data is present.
 */
class COzzKinematicsVisual final : public FHierrarhyVisual
{
public:
    COzzKinematicsVisual();
    ~COzzKinematicsVisual() override;

    void Load(const char* N, IReader* data, u32 dwFlags) override;
    void Copy(dxRender_Visual* pFrom) override;
    void Spawn() override;
    void Depart() override;
    void Release() override;

    bool LoadFromBundle(const char* name, const std::filesystem::path& path);

    // Interface casts
    IKinematics* dcast_PKinematics() override;
    IKinematicsAnimated* dcast_PKinematicsAnimated() override;
    COzzKinematicsVisual* dcast_OzzKinematics() override { return this; }

    // Access to kinematics - returns base pointer (always valid after initialization)
    OzzKinematics* Kinematics() { return kinematics_.get(); }
    const OzzKinematics* Kinematics() const { return kinematics_.get(); }

    // Access to animated kinematics (may return nullptr for static models)
    OzzKinematicsAnimated* AnimatedKinematics();
    const OzzKinematicsAnimated* AnimatedKinematics() const;

    bool HasGeometry() const { return !meshes_.empty(); }
    bool IsKinematicsReady() const;
    bool IsAnimated() const { return is_animated_; }

    const xr_vector<Fmatrix>& SkinningPalette();
    const xr_vector<ozz::sample::Mesh>& Meshes() const { return meshes_; }

    // Animation helpers (forward to animated kinematics if present)
    bool PlayMotion(const xr_string& motion_name);
    xr_vector<xr_string> GetAvailableMotions();
    bool LoadAnimationFromFile(const std::filesystem::path& path);
    void StopAnimation();

    void EnsureSkinningPalette();
    void OnPoseUpdated();
    void UpdateSkinningPalette();
    static void HandleKinematicsUpdated(IKinematics* kin);

    void DebugDumpPalette(const xr_vector<Fmatrix>& palette) const override;

private:
    void UpdateBounds();
    void DestroySurfaces();
    bool UpdateAnimation(float dt);
    bool InitializeFromPayload(bool spawn_children = false);
    bool RequiresAnimation() const;

private:
    // Bundle data
    xr_vector<std::uint8_t> skeleton_payload_;
    xr_vector<std::uint8_t> mesh_payload_;
    std::vector<std::uint8_t> user_data_payload_;
    std::vector<std::uint8_t> embedded_animation_payload_;
    xr_vector<ozz::sample::Mesh> meshes_;
    xr_vector<COzzSkinnedSurface*> surfaces_;
    xr_vector<Fmatrix> bone_palette_;
    xr_vector<xr_string> motion_references_;
    XRay::Animation::ExtendedBoneMetadataCollection bone_metadata_;

    // State
    u32 last_animation_update_frame_ = u32(-1);
    bool initialized_ = false;
    bool is_animated_ = false;

    // Kinematics - holds either static or animated version
    xr_unique_ptr<OzzKinematics> kinematics_;
    // Cached pointer to animated kinematics (same as kinematics_ if animated, nullptr otherwise)
    OzzKinematicsAnimated* animated_kinematics_ = nullptr;
};

} // namespace xray::render::RENDER_NAMESPACE
