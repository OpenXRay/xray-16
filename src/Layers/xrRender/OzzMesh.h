#pragma once

#include "FVisual.h"

#include "ozz/base/maths/simd_math.h"

#include <cstdint>
#include <vector>

namespace ozz::sample { struct Mesh; }

namespace XRay::Animation { class OzzKinematics; }

namespace xray::render::fg
{
class OzzMesh : public Fvisual
{
public:
    OzzMesh();
    ~OzzMesh() override;

    void Copy(dxRender_Visual* pFrom) override;

    void LoadFromOzzMesh(XRay::Animation::OzzKinematics* parent, const ozz::sample::Mesh& mesh);

    void SetParent(XRay::Animation::OzzKinematics* parent) { m_Parent = parent; }
    XRay::Animation::OzzKinematics* GetParent() const { return m_Parent; }

    const std::vector<std::uint16_t>& GetJointRemaps() const { return m_jointRemaps; }
    const std::vector<ozz::math::Float4x4>& GetInverseBindPoses() const { return m_inverseBindPoses; }
    std::vector<Fmatrix>& PaletteStaging() { return m_paletteStaging; }

    u32 GetBoneCount() const { return m_boneCount; }
    u32 GetUploadedBoneOffset() const { return m_uploadedBoneOffset; }
    void SetUploadedBoneOffset(u32 offset) { m_uploadedBoneOffset = offset; }
    u64 GetUploadedFrame() const { return m_uploadedFrame; }
    void SetUploadedFrame(u64 frame) { m_uploadedFrame = frame; }

private:
    XRay::Animation::OzzKinematics* m_Parent = nullptr;

    std::vector<std::uint16_t> m_jointRemaps;
    std::vector<ozz::math::Float4x4> m_inverseBindPoses;
    std::vector<Fmatrix> m_paletteStaging;

    u32 m_boneCount = 0;
    u32 m_uploadedBoneOffset = 0;
    u64 m_uploadedFrame = 0;
};
}
