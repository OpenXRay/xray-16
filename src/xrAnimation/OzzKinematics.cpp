#include "stdafx.h"
#include "OzzKinematics.h"
#include "OzzConversion.h"
#include "xrEngine/device.h"

namespace XRay::Animation
{
OzzKinematics::OzzKinematics() : stubBoneData(u16(-1))
{
    stubBoneInstance.construct();
    core.SetOwner(this);
}

OzzKinematics::~OzzKinematics() {}

bool OzzKinematics::InitializeFromOzz(pcstr skeletonPath)
{
    bool result = core.InitializeFromOzz(skeletonPath);
    if (result)
        OnSkeletonLoaded();
    return result;
}

bool OzzKinematics::InitializeFromOzzBuffer(ozz::span<const std::byte> skeletonData)
{
    bool result = core.InitializeFromOzzBuffer(skeletonData);
    if (result)
        OnSkeletonLoaded();
    return result;
}

bool OzzKinematics::ApplyExtendedBoneMetadata(const ExtendedBoneMetadataCollection& metadata)
{
    return core.ApplyExtendedBoneMetadata(metadata);
}

bool OzzKinematics::LoadUserDataFromBuffer(const std::vector<std::uint8_t>& buffer)
{
    return core.LoadUserDataFromBuffer(buffer);
}

bool OzzKinematics::SetPoseLocals(ozz::span<const ozz::math::SoaTransform> locals)
{
    return core.SetPoseLocals(locals);
}

void OzzKinematics::ClearPose()
{
    core.ClearPose();
}

void OzzKinematics::BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const
{
    core.BuildSkinningPalette(out_matrices, render_space);
}

void OzzKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
    core.CalculateTransforms(TRUE);
}

void OzzKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    if (!core.IsInitialized() || id >= core.GetBoneCount())
    {
        pos.identity();
        return;
    }

    core.CalculateTransforms(TRUE);
    pos = core.GetBoneTransform(id);
}

bool OzzKinematics::PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    return false;
}

void OzzKinematics::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) {}

u16 OzzKinematics::LL_BoneID(LPCSTR B)
{
    return core.FindBoneID(B);
}

u16 OzzKinematics::LL_BoneID(const shared_str& B)
{
    return core.FindBoneID(B);
}

LPCSTR OzzKinematics::LL_BoneName_dbg(u16 ID)
{
    return core.GetBoneName(ID);
}

CInifile* OzzKinematics::LL_UserData()
{
    return core.GetUserData();
}

IKinematics::accel* OzzKinematics::LL_Bones()
{
    return core.GetBoneMapByName();
}

CBoneInstance& OzzKinematics::LL_GetBoneInstance(u16 bone_id)
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.instance)
        return *info.instance;

    NotImplemented(__FUNCTION__);
    return StubBoneInstance();
}

CBoneData& OzzKinematics::LL_GetData(u16 bone_id)
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.data)
        return *info.data;

    return StubBoneData();
}

const IBoneData& OzzKinematics::GetBoneData(u16 bone_id) const
{
    OzzKinematicsCore::BoneInfo info;
    if (core.GetBoneInfo(bone_id, info) && info.data)
        return *info.data;

    return StubBoneData();
}

u16 OzzKinematics::LL_BoneCount() const
{
    return core.GetBoneCount();
}

u16 OzzKinematics::LL_VisibleBoneCount()
{
    u16 count = 0;
    const u16 total = core.GetBoneCount();
    for (u16 i = 0; i < total; ++i)
    {
        if (core.IsBoneVisible(i))
            ++count;
    }
    return count;
}

Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id)
{
    if (bone_id < core.GetBoneCount())
        return const_cast<Fmatrix&>(core.GetBoneTransform(bone_id));

    NotImplemented(__FUNCTION__);
    return StubMatrix();
}

const Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id) const
{
    if (bone_id < core.GetBoneCount())
        return core.GetBoneTransform(bone_id);

    const_cast<OzzKinematics*>(this)->NotImplemented(__FUNCTION__);
    return StubMatrix();
}

Fmatrix& OzzKinematics::LL_GetTransform_R(u16 bone_id)
{
    if (bone_id < core.GetBoneCount())
        return const_cast<Fmatrix&>(core.GetBoneRenderTransform(bone_id));

    NotImplemented(__FUNCTION__);
    return StubMatrix();
}

Fobb& OzzKinematics::LL_GetBox(u16 bone_id)
{
    return core.GetBoneBox(bone_id);
}

const Fbox& OzzKinematics::GetBox() const
{
    return core.GetBoundingBox();
}

void OzzKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    core.GetBindTransforms(matrices);
}

int OzzKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
    groups.clear();
    return 0;
}

u16 OzzKinematics::LL_GetBoneRoot()
{
    return core.GetRootBone();
}

void OzzKinematics::LL_SetBoneRoot(u16 bone_id)
{
    core.SetRootBone(bone_id);
}

BOOL OzzKinematics::LL_GetBoneVisible(u16 bone_id)
{
    return core.IsBoneVisible(bone_id) ? TRUE : FALSE;
}

void OzzKinematics::LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive)
{
    core.SetBoneVisible(bone_id, val != FALSE, bRecursive != FALSE);
}

u64 OzzKinematics::LL_GetBonesVisible()
{
    return core.GetVisibilityMask();
}

void OzzKinematics::LL_SetBonesVisible(u64 mask)
{
    core.SetVisibilityMask(mask);
}

void OzzKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    core.AddBoneTransform(offset);
}

void OzzKinematics::LL_ClearAdditionalTransform(u16 bone_id)
{
    core.ClearBoneTransform(bone_id);
}

void OzzKinematics::CalculateBones(BOOL bForceExact)
{
    core.CalculateTransforms(bForceExact);
}

void OzzKinematics::CalculateBones_Invalidate()
{
    core.InvalidateCache();
}

void OzzKinematics::Callback(UpdateCallback C, void* Param)
{
    core.SetUpdateCallback(C, Param);
}

void OzzKinematics::SetUpdateCallback(UpdateCallback pCallback)
{
    core.SetUpdateCallback(pCallback, core.GetUpdateCallbackParam());
}

void OzzKinematics::SetUpdateCallbackParam(void* pCallbackParam)
{
    core.SetUpdateCallback(core.GetUpdateCallback(), pCallbackParam);
}

UpdateCallback OzzKinematics::GetUpdateCallback()
{
    return core.GetUpdateCallback();
}

void* OzzKinematics::GetUpdateCallbackParam()
{
    return core.GetUpdateCallbackParam();
}

#ifdef DEBUG
void OzzKinematics::DebugRender(Fmatrix& XFORM)
{
    NotImplemented(__FUNCTION__);
}

shared_str OzzKinematics::getDebugName()
{
    return shared_str("OzzKinematics");
}
#endif

void OzzKinematics::NotImplemented(pcstr function_name) const
{
    Msg("[OzzKinematics] %s is not implemented for static models", function_name);
}

CBoneInstance& OzzKinematics::StubBoneInstance() const
{
    return stubBoneInstance;
}

CBoneData& OzzKinematics::StubBoneData() const
{
    return stubBoneData;
}

Fmatrix& OzzKinematics::StubMatrix() const
{
    return stubMatrix;
}

Fobb& OzzKinematics::StubObb() const
{
    return stubObb;
}
} // namespace XRay::Animation
