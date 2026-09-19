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

namespace XRay::Animation
{
OzzKinematics::OzzKinematics()
    : poseValid(false), rootBone(BI_NONE), lastUpdateTime(0), updateCallback(nullptr), updateCallbackParam(nullptr)
{
    cachedBox.invalidate();
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

    boneBoxes.resize(count);
    for (Fobb& box : boneBoxes)
        box.invalidate();

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
    cachedBox.invalidate();
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
    CalculateBones(TRUE);
}

void OzzKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 channel_mask, bool ignore_callbacks)
{
    if (!model || id >= LL_BoneCount())
    {
        pos.identity();
        return;
    }

    CalculateBones(TRUE);
    pos = boneInstances[id].mTransform;
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
    static Fobb stub;
    if (bone_id < boneBoxes.size())
        return boneBoxes[bone_id];
    return stub;
}

const Fbox& OzzKinematics::GetBox() const
{
    auto* self = const_cast<OzzKinematics*>(this);

    if (!model || boneInstances.empty())
    {
        self->cachedBox.invalidate();
        return cachedBox;
    }

    Fbox computed;
    computed.invalidate();

    for (size_t idx = 0; idx < boneInstances.size(); ++idx)
    {
        if (!IsBoneVisible(static_cast<u16>(idx)))
            continue;

        const Fmatrix& transform = boneInstances[idx].mTransform;
        computed.modify(transform.c);
    }

    if (!computed.is_valid())
        computed.set_zero();

    self->cachedBox = computed;
    return cachedBox;
}

void OzzKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    matrices.clear();
    if (!model)
        return;

    matrices.reserve(model->bones.size());
    for (CBoneData* bone : model->bones)
    {
        if (bone)
            matrices.push_back(bone->bind_transform);
    }
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

void OzzKinematics::CalculateBones(BOOL)
{
    if (!model || boneInstances.empty())
        return;

    const ozz::animation::Skeleton& skeleton = model->skeleton;
    if (skeleton.num_joints() == 0)
        return;

    const u32 currentTime = Device.dwTimeGlobal;
    if (currentTime == lastUpdateTime)
        return;

    OnCalculateBones();
    lastUpdateTime = currentTime;

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

    Fbox box;
    box.invalidate();

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
            box.modify(inst.mTransform.c);
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

    cachedBox = box;

    if (updateCallback)
        updateCallback(this);
}

void OzzKinematics::CalculateBones_Invalidate()
{
    lastUpdateTime = 0;
    cachedBox.invalidate();
}

#ifdef DEBUG
void OzzKinematics::DebugRender(Fmatrix& XFORM) {}

shared_str OzzKinematics::getDebugName()
{
    return shared_str("OzzKinematics");
}
#endif
}
