#include "stdafx.h"

#include "OzzKinematics.h"

#include "OzzConversion.h"
#include "OzzMesh.h"

#include "framework/mesh.h"

#include "xrAnimation/OzzBundle.h"
#include "xrCore/FS.h"
#include "xrCore/FS_impl.h"
#include "xrEngine/device.h"

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/maths/simd_math.h"

#include <algorithm>
#include <memory>

namespace xray::render::fg
{
extern int psSkeletonUpdate;
}

namespace XRay::Animation
{
OzzKinematics::OzzKinematics()
    : poseValid(false), rootBone(BI_NONE), lastUpdateTime(0), UCalc_Visibox(xray::render::fg::psSkeletonUpdate), updateCallback(nullptr),
      updateCallbackParam(nullptr)
{
}

OzzKinematics::~OzzKinematics()
{
    updateCallback = nullptr;
    updateCallbackParam = nullptr;
}

void OzzKinematics::Load(const char*, IReader*, u32)
{
    R_ASSERT2(false, "OzzKinematics::Load not yet implemented");
}

bool OzzKinematics::LoadBundle(const OzzxBundle& bundle)
{
    model.reset();

    auto data = xr_make_shared<OzzModelData>();
    if (!data->Load(bundle))
        return false;

    model = std::move(data);

    IBoneInstances_Create();
    return true;
}

u64 OzzKinematics::FullVisibilityMask() const
{
    const size_t count = model ? model->bones.size() : 0;
    if (count == 0)
        return 0;
    if (count >= 64)
        return u64(-1);
    return (u64(1) << count) - 1;
}

void OzzKinematics::IBoneInstances_Create()
{
    const size_t count = model ? model->bones.size() : 0;

    boneInstances.resize(count);
    for (CBoneInstance& instance : boneInstances)
        instance.construct();

    modelTransforms.resize(count);
    subtreeDelta.resize(count);
    subtreeDirty.assign(count, 0);

    sampledLocals.resize(model ? static_cast<size_t>(model->skeleton.num_soa_joints()) : 0);
    for (ozz::math::SoaTransform& transform : sampledLocals)
        transform = ozz::math::SoaTransform::identity();
    poseValid = false;

    boneVisibility.setAll(FullVisibilityMask());

    rootBone = model ? model->defaultRoot : BI_NONE;
    boneOffsets.clear();
    lastUpdateTime = 0;
    UCalc_Visibox = xray::render::fg::psSkeletonUpdate;
}

bool OzzKinematics::LoadMeshFromBuffer(const std::vector<std::uint8_t>& meshData)
{
    if (meshData.empty())
        return false;

    ozz::io::MemoryStream stream;
    stream.Write(meshData.data(), static_cast<size_t>(meshData.size()));
    stream.Seek(0, ozz::io::Stream::kSet);

    ozz::io::IArchive archive(&stream);
    bool any_loaded = false;
    Fbox merged_box;
    merged_box.invalidate();

    while (archive.TestTag<ozz::sample::Mesh>())
    {
        ozz::sample::Mesh mesh;
        archive >> mesh;

        if (mesh.vertex_count() <= 0 || mesh.triangle_index_count() <= 0)
            continue;

        auto* meshVisual = xr_new<xray::render::fg::OzzMesh>();
        meshVisual->LoadFromOzzMesh(this, mesh);
#ifdef DEBUG
        if (dbg_name.c_str())
            meshVisual->dbg_name = dbg_name;
#endif
        children.push_back(meshVisual);
        merged_box.merge(meshVisual->vis.box);
        any_loaded = true;
    }

    if (any_loaded)
    {
        vis.box.set(merged_box);
        Fvector center; center.add(merged_box.vMin, merged_box.vMax); center.mul(0.5f);
        Fvector extent; extent.sub(merged_box.vMax, merged_box.vMin); extent.mul(0.5f);
        vis.sphere.P = center;
        vis.sphere.R = extent.magnitude();
    }
    return any_loaded;
}

void OzzKinematics::Copy(xray::render::fg::dxRender_Visual* pFrom)
{
    xray::render::fg::FHierrarhyVisual::Copy(pFrom);

    auto* src = static_cast<OzzKinematics*>(pFrom);

    model = src->model;

    IBoneInstances_Create();

    boneVisibility.setAll(src->boneVisibility.mask);
    rootBone = src->rootBone;

    for (auto* child : children)
    {
        if (auto* mesh_child = dynamic_cast<xray::render::fg::OzzMesh*>(child))
            mesh_child->SetParent(this);
    }

    CalculateBones_Invalidate();
}

void OzzKinematics::Spawn()
{
    xray::render::fg::FHierrarhyVisual::Spawn();

    for (CBoneInstance& instance : boneInstances)
        instance.construct();

    updateCallback = nullptr;
    updateCallbackParam = nullptr;

    CalculateBones_Invalidate();

    rootBone = model ? model->defaultRoot : BI_NONE;
}

void OzzKinematics::Depart()
{
    xray::render::fg::FHierrarhyVisual::Depart();

    boneVisibility.setAll(FullVisibilityMask());
}

void OzzKinematics::ClearPose()
{
    if (!model)
        return;

    sampledLocals.clear();
    poseValid = false;
    CalculateBones_Invalidate();
}

void OzzKinematics::BuildSkinningPalette(xr_vector<Fmatrix>& out_matrices, bool render_space) const
{
    if (!model)
    {
        out_matrices.clear();
        return;
    }

    const size_t expected = static_cast<size_t>(model->skeleton.num_joints());
    if (expected == 0)
    {
        out_matrices.clear();
        return;
    }

    const size_t instance_count = boneInstances.size();
    const size_t model_count = modelTransforms.size();
    const size_t available = std::min(instance_count, std::min(model_count, expected));

    out_matrices.resize(expected);

    if (available != expected)
    {
        Msg("[OzzKinematics] BuildSkinningPalette requested before bone buffers ready (instances=%zu, model=%zu, expected=%zu)", instance_count,
            model_count, expected);

        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = Fidentity;
        return;
    }

    if (render_space)
    {
        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = boneInstances[idx].mRenderTransform;
    }
    else
    {
        for (size_t idx = 0; idx < expected; ++idx)
            out_matrices[idx] = boneInstances[idx].mTransform;
    }
}

bool OzzKinematics::IsBoneVisible(u16 bone_id) const
{
    if (bone_id >= boneInstances.size())
        return false;
    return boneVisibility.get(bone_id);
}

bool OzzKinematics::ApplyAdditionalBoneTransforms(u16 bone_id, Fmatrix& transform) const
{
    bool applied = false;
    for (const auto& offset : boneOffsets)
    {
        if (offset.m_bone_id != bone_id)
            continue;

        const Fvector original_position = transform.c;
        transform.mulB_43(offset.m_transform);
        transform.c.add(original_position, offset.m_transform.c);
        applied = true;
    }
    return applied;
}

void OzzKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
    if (!bd || !model || boneInstances.empty())
        return;

    const u16 self_id = bd->GetSelfID();
    const size_t count = std::min(boneInstances.size(), modelTransforms.size());
    if (self_id >= count)
        return;

    const ozz::span<const int16_t> parents = model->skeleton.joint_parents();

    size_t end = static_cast<size_t>(self_id) + 1;
    while (end < count && parents[end] >= static_cast<int16_t>(self_id))
        ++end;

    const size_t range = end - static_cast<size_t>(self_id);
    boneCalcScratch.resize(range);
    for (size_t k = 0; k < range; ++k)
        boneCalcScratch[k] = ConvertOzzMatrixToXRay(modelTransforms[static_cast<size_t>(self_id) + k]);

    for (size_t j = self_id; j < end; ++j)
    {
        CBoneInstance& bi = boneInstances[j];
        if (!IsBoneVisible(static_cast<u16>(j)))
            continue;

        const int16_t parent_index = parents[j];
        const Fmatrix& parent_matrix = (j == static_cast<size_t>(self_id))
            ? (parent ? *parent : Fidentity)
            : boneInstances[parent_index].mTransform;

        const Fmatrix& anim = boneCalcScratch[j - static_cast<size_t>(self_id)];

        Fmatrix local;
        if (parent_index >= 0)
        {
            const Fmatrix parent_anim = (static_cast<size_t>(parent_index) >= static_cast<size_t>(self_id))
                ? boneCalcScratch[static_cast<size_t>(parent_index) - static_cast<size_t>(self_id)]
                : ConvertOzzMatrixToXRay(modelTransforms[parent_index]);
            Fmatrix parent_inverse;
            parent_inverse.invert(parent_anim);
            local.mul_43(parent_inverse, anim);
        }
        else
        {
            local = anim;
        }

        if (bi.callback_overwrite())
        {
            if (bi.callback())
                bi.callback()(&bi);
        }
        else
        {
            bi.mTransform.mul_43(parent_matrix, local);
            ApplyAdditionalBoneTransforms(static_cast<u16>(j), bi.mTransform);
            if (bi.callback())
                bi.callback()(&bi);
        }

        bi.mRenderTransform.mul_43(bi.mTransform, model->bones[j]->m2b_transform);

        Fmatrix anim_inverse;
        anim_inverse.invert(anim);
        subtreeDirty[j] = 1;
        subtreeDelta[j].mul_43(bi.mTransform, anim_inverse);
    }
}

void OzzKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    const size_t count = std::min(boneInstances.size(), modelTransforms.size());
    if (!model || id >= count)
    {
        pos.identity();
        return;
    }

    const ozz::span<const int16_t> parents = model->skeleton.joint_parents();

    boneChainScratch.clear();
    for (int16_t node = static_cast<int16_t>(id);;)
    {
        boneChainScratch.push_back(static_cast<u16>(node));
        if (static_cast<u16>(node) == rootBone || parents[node] < 0)
            break;
        node = parents[node];
    }

    Fmatrix parentTemp = Fidentity;
    for (size_t k = boneChainScratch.size(); k > 0; --k)
    {
        const u16 b = boneChainScratch[k - 1];
        CBoneInstance bi = boneInstances[b];

        if (!ignore_callbacks && bi.callback_overwrite())
        {
            if (bi.callback())
                bi.callback()(&bi);
        }
        else
        {
            const Fmatrix anim = ConvertOzzMatrixToXRay(modelTransforms[b]);
            const int16_t parent_index = parents[b];

            Fmatrix local;
            if (parent_index >= 0)
            {
                Fmatrix parent_inverse;
                parent_inverse.invert(ConvertOzzMatrixToXRay(modelTransforms[parent_index]));
                local.mul_43(parent_inverse, anim);
            }
            else
            {
                local = anim;
            }

            bi.mTransform.mul_43(parentTemp, local);
            ApplyAdditionalBoneTransforms(b, bi.mTransform);
            if (!ignore_callbacks && bi.callback())
                bi.callback()(&bi);
        }

        parentTemp = bi.mTransform;
    }

    pos = parentTemp;
}

bool OzzKinematics::PickBone(const Fmatrix& parent_xform, pick_result& r, float dist, const Fvector& start, const Fvector& dir, u16 bone_id)
{
    return false;
}

void OzzKinematics::EnumBoneVertices(SEnumVerticesCallback& C, u16 bone_id) {}

u16 OzzKinematics::LL_BoneID(LPCSTR B)
{
    if (!B || !model)
        return BI_NONE;

    const accel& map = model->boneMapByName;

    const auto it = std::lower_bound(map.begin(), map.end(), B,
        [](const accel::value_type& entry, pcstr value)
        {
            return xr_strcmp(entry.first.c_str(), value) < 0;
        });

    if (it == map.end() || xr_strcmp(it->first.c_str(), B) != 0)
        return BI_NONE;
    return it->second;
}

u16 OzzKinematics::LL_BoneID(const shared_str& B)
{
    if (!B._get() || !model)
        return BI_NONE;

    const accel& map = model->boneMapByPtr;

    const auto it = std::lower_bound(map.begin(), map.end(), B,
        [](const accel::value_type& entry, const shared_str& value)
        {
            return entry.first._get() < value._get();
        });

    if (it != map.end() && it->first._get() == B._get())
        return it->second;

    return LL_BoneID(B.c_str());
}

LPCSTR OzzKinematics::LL_BoneName_dbg(u16 ID)
{
    if (!model || ID >= boneInstances.size())
        return nullptr;

    const accel& map = model->boneMapByName;

    const auto it = std::find_if(map.begin(), map.end(),
        [ID](const accel::value_type& entry)
        {
            return entry.second == ID;
        });

    return it != map.end() ? it->first.c_str() : nullptr;
}

CBoneInstance& OzzKinematics::LL_GetBoneInstance(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetBoneInstance: bone id out of range");
    return boneInstances[bone_id];
}

CBoneData& OzzKinematics::LL_GetData(u16 bone_id)
{
    R_ASSERT2(model && bone_id < model->bones.size() && model->bones[bone_id], "OzzKinematics::LL_GetData: bone id out of range");
    return *model->bones[bone_id];
}

const IBoneData& OzzKinematics::GetBoneData(u16 bone_id) const
{
    R_ASSERT2(model && bone_id < model->bones.size() && model->bones[bone_id], "OzzKinematics::GetBoneData: bone id out of range");
    return *model->bones[bone_id];
}

u16 OzzKinematics::LL_VisibleBoneCount()
{
    u16 count = 0;
    const u16 total = LL_BoneCount();
    for (u16 i = 0; i < total; ++i)
    {
        if (IsBoneVisible(i))
            ++count;
    }
    return count;
}

Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform: bone id out of range");
    return boneInstances[bone_id].mTransform;
}

const Fmatrix& OzzKinematics::LL_GetTransform(u16 bone_id) const
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform: bone id out of range");
    return boneInstances[bone_id].mTransform;
}

Fmatrix& OzzKinematics::LL_GetTransform_R(u16 bone_id)
{
    R_ASSERT2(bone_id < boneInstances.size(), "OzzKinematics::LL_GetTransform_R: bone id out of range");
    return boneInstances[bone_id].mRenderTransform;
}

Fobb& OzzKinematics::LL_GetBox(u16 bone_id)
{
    R_ASSERT2(model && bone_id < model->bones.size() && model->bones[bone_id], "OzzKinematics::LL_GetBox: bone id out of range");
    return model->bones[bone_id]->obb;
}

const Fbox& OzzKinematics::GetBox() const
{
    return vis.box;
}

void OzzKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    const u16 count = LL_BoneCount();
    matrices.resize(count);
    if (!model)
        return;

    const size_t available = std::min(static_cast<size_t>(count), model->bindModel.size());
    for (size_t idx = 0; idx < available; ++idx)
        matrices[idx] = model->bindModel[idx];
}

int OzzKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
    groups.clear();
    return 0;
}

BOOL OzzKinematics::LL_GetBoneVisible(u16 bone_id)
{
    return IsBoneVisible(bone_id) ? TRUE : FALSE;
}

void OzzKinematics::LL_SetBoneVisible(u16 bone_id, BOOL val, BOOL bRecursive)
{
    if (bone_id >= boneInstances.size())
        return;

    boneVisibility.set(bone_id, val != FALSE);

    if (val != FALSE)
    {
        CalculateBones_Invalidate();
    }
    else
    {
        boneInstances[bone_id].mTransform.scale(0.f, 0.f, 0.f);
        boneInstances[bone_id].mRenderTransform.scale(0.f, 0.f, 0.f);
    }

    if (bRecursive != FALSE && model && bone_id < model->bones.size() && model->bones[bone_id])
    {
        for (const CBoneData* child : model->bones[bone_id]->children)
            LL_SetBoneVisible(child->GetSelfID(), val, bRecursive);
    }
}

u64 OzzKinematics::LL_GetBonesVisible()
{
    return boneVisibility.mask;
}

void OzzKinematics::LL_SetBonesVisible(u64 mask)
{
    boneVisibility.setAll(mask);

    const size_t count = boneInstances.size();
    for (size_t idx = 0; idx < count && idx < 64; ++idx)
    {
        const u64 bit = u64(1) << idx;
        if ((mask & bit) == 0)
        {
            boneInstances[idx].mTransform.scale(0.f, 0.f, 0.f);
            boneInstances[idx].mRenderTransform.scale(0.f, 0.f, 0.f);
        }
    }
    CalculateBones_Invalidate();
}

void OzzKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& offset)
{
    boneOffsets.push_back(offset);
    CalculateBones_Invalidate();
}

void OzzKinematics::LL_ClearAdditionalTransform(u16 bone_id)
{
    if (bone_id == BI_NONE)
    {
        boneOffsets.clear();
    }
    else
    {
        boneOffsets.erase(std::remove_if(boneOffsets.begin(), boneOffsets.end(),
                              [bone_id](const KinematicsABT::additional_bone_transform& entry)
                              {
                                  return entry.m_bone_id == bone_id;
                              }),
            boneOffsets.end());
    }

    CalculateBones_Invalidate();
}

void OzzKinematics::CalculateBones(BOOL bForceExact)
{
    if (!model || boneInstances.empty())
        return;

    const ozz::animation::Skeleton& skeleton = model->skeleton;
    if (skeleton.num_joints() == 0)
        return;

    if (Device.dwTimeGlobal == lastUpdateTime)
        return;

    OnCalculateBones();

    if (!bForceExact && (Device.dwTimeGlobal < (lastUpdateTime + UCalc_Interval)))
        return;

    lastUpdateTime = Device.dwTimeGlobal;

    const size_t jointCount = static_cast<size_t>(skeleton.num_joints());

    ozz::span<const ozz::math::SoaTransform> input =
        (poseValid && !sampledLocals.empty()) ? ozz::make_span(sampledLocals) : skeleton.joint_rest_poses();

    ozz::animation::LocalToModelJob job;
    job.skeleton = &skeleton;
    job.input = input;
    job.output = ozz::span<ozz::math::Float4x4>(modelTransforms.data(), modelTransforms.size());
    if (!job.Run())
        return;

    const ozz::span<const int16_t> parents = skeleton.joint_parents();

    for (size_t i = 0; i < jointCount; ++i)
    {
        CBoneInstance& inst = boneInstances[i];
        const Fmatrix anim = ConvertOzzMatrixToXRay(modelTransforms[i]);
        const int16_t parent = parents[i];
        const bool inherited = parent >= 0 && subtreeDirty[parent] != 0;

        bool modified = false;
        if (!IsBoneVisible(static_cast<u16>(i)))
        {
            inst.mTransform.scale(0.f, 0.f, 0.f);
            inst.mRenderTransform.scale(0.f, 0.f, 0.f);
            modified = true;
        }
        else
        {
            if (inst.callback_overwrite())
            {
                if (inst.callback())
                    inst.callback()(&inst);
                modified = true;
            }
            else
            {
                if (inherited)
                    inst.mTransform.mul_43(subtreeDelta[parent], anim);
                else
                    inst.mTransform = anim;
                modified = ApplyAdditionalBoneTransforms(static_cast<u16>(i), inst.mTransform);
                if (inst.callback())
                {
                    inst.callback()(&inst);
                    modified = true;
                }
            }

            inst.mRenderTransform.mul_43(inst.mTransform, model->bones[i]->m2b_transform);
        }

        subtreeDirty[i] = (modified || inherited) ? 1 : 0;
        if (modified)
        {
            Fmatrix inverse;
            inverse.invert(anim);
            subtreeDelta[i].mul_43(inst.mTransform, inverse);
        }
        else if (inherited)
        {
            subtreeDelta[i] = subtreeDelta[parent];
        }
    }

    UCalc_Visibox++;
    if (UCalc_Visibox >= xray::render::fg::psSkeletonUpdate)
    {
        UCalc_Visibox = -(::Random.randI(xray::render::fg::psSkeletonUpdate - 1));

        Fbox Box;
        Box.invalidate();
        for (size_t b = 0; b < jointCount; ++b)
        {
            if (!IsBoneVisible(static_cast<u16>(b)))
                continue;

            Fobb& obb = model->bones[b]->obb;
            Fmatrix& Mbone = boneInstances[b].mTransform;
            Fmatrix Mbox;
            obb.xform_get(Mbox);
            Fmatrix X;
            X.mul_43(Mbone, Mbox);
            Fvector& S = obb.m_halfsize;

            Fvector P, A;
            A.set(-S.x, -S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, -S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, -S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, -S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(-S.x, S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, S.y, S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
            A.set(S.x, S.y, -S.z);
            X.transform_tiny(P, A);
            Box.modify(P);
        }
        if (jointCount)
        {
            vis.box.vMin = (Box.vMin);
            vis.box.vMax = (Box.vMax);
            vis.box.getsphere(vis.sphere.P, vis.sphere.R);
        }
    }

    if (updateCallback)
        updateCallback(this);
}

void OzzKinematics::CalculateBones_Invalidate()
{
    lastUpdateTime = 0;
    UCalc_Visibox = xray::render::fg::psSkeletonUpdate;
}

#ifdef DEBUG
void OzzKinematics::DebugRender(Fmatrix& XFORM) {}

shared_str OzzKinematics::getDebugName()
{
    return shared_str("OzzKinematics");
}
#endif
}
