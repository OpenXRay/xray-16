#include "xrEngine/stdafx.h"
#include "VulkanKinematics.h"
#include "xrCore/FS.h"
#include "xrCore/xr_ini.h"
#include "xrEngine/EnnumerateVertices.h"
#include "xrEngine/vis_common.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace xray::render::vulkan
{
struct VulkanKinematics::Data
{
    std::vector<std::unique_ptr<CBoneData>> owned;
    vecBones bones;
    accel names;
    u16 root = BI_NONE;
    std::unique_ptr<CInifile> user_data;
    std::vector<ModelGeometry> meshes;
    std::vector<MotionSlot> motions;
    // Legacy motion objects back the low-level IKinematicsAnimated queries.
    // Destroy handles before their private container (reverse field order).
    std::unique_ptr<motions_container> motion_container;
    std::vector<shared_motions> legacy_motions;
};

void VulkanKinematics::set_motions(std::vector<MotionSlot> motions)
{
    data_->motions = std::move(motions);
    playback_.reset();
    if (data_->motions.empty()) return;
    const auto& first = data_->motions.front();
    for (size_t part = 0; part < first.partitions.size(); ++part)
    {
        partitions_[static_cast<u16>(part)].Name = first.partition_names[part].c_str();
        for (uint16_t bone : first.partitions[part])
            partitions_[static_cast<u16>(part)].bones.push_back(bone);
    }
    data_->legacy_motions.clear();
    data_->motion_container = std::make_unique<motions_container>();
    for (size_t index = 0; index < data_->motions.size(); ++index)
    {
        const auto& slot = data_->motions[index];
        if (slot.raw.empty())
        {
            data_->legacy_motions.emplace_back();
            continue;
        }
        IReader reader(const_cast<uint8_t*>(slot.raw.data()), slot.raw.size());
        shared_motions legacy;
        auto* previous = g_pMotionsContainer;
        g_pMotionsContainer = data_->motion_container.get();
        const bool loaded = legacy.create(slot.source.c_str(), &reader, &data_->bones);
        g_pMotionsContainer = previous;
        R_ASSERT2(loaded, "validated Vulkan motion source failed legacy interface loading");
        data_->legacy_motions.push_back(std::move(legacy));
    }
}

const std::vector<MotionSlot>& VulkanKinematics::motions() const
{
    return data_->motions;
}

bool VulkanKinematics::play_motion(const std::string& name, bool fx, bool mixing,
    float power, MotionPlayback::Finished finished)
{
    MotionPlayback::Handle id;
    if (!playback_.find(name, fx, id) ||
        !playback_.play(id, fx, mixing, power, std::move(finished))) return false;
    dirty_ = true;
    return true;
}

void VulkanKinematics::advance_motions(float seconds)
{
    if (!playback_.active_count()) return;
    playback_.advance(seconds);
    dirty_ = true;
}

void VulkanKinematics::stop_motions(uint16_t part)
{
    playback_.stop_cycles(part);
    dirty_ = true;
}

VulkanKinematics::VulkanKinematics(std::shared_ptr<Data> data, IRenderVisual* owner)
    : data_(std::move(data)), owner_(owner), instances_(data_->bones.size()),
      playback_(std::shared_ptr<const std::vector<MotionSlot>>(data_, &data_->motions), [&]
      {
          std::vector<uint16_t> parents;
          parents.reserve(data_->bones.size());
          for (const auto* bone : data_->bones) parents.push_back(bone->GetParentID());
          return parents;
      }())
{
    for (auto& instance : instances_) instance.construct();
    visible_ = instances_.size() == 64 ? ~u64(0) : (u64(1) << instances_.size()) - 1;
}

VulkanKinematics::VulkanKinematics(const VulkanKinematics& source, IRenderVisual* owner)
    : data_(source.data_), owner_(owner), instances_(source.instances_), offsets_(source.offsets_),
      playback_(source.playback_), partitions_(source.partitions_), root_(source.root_),
      visible_(source.visible_), dirty_(source.dirty_)
{
    // Callbacks commonly carry a pointer into the old model instance. Copies
    // start without it and may install their own callback after creation.
    for (auto& instance : instances_) instance.reset_callback();
    playback_.clear_callbacks();
    for (const auto& old : source.blends_)
        if (old->blend_state() != CBlend::eFREE_SLOT)
        {
            auto blend = std::make_unique<CBlend>(*old);
            blend->Callback = nullptr;
            blend->CallbackParam = nullptr;
            blends_.push_back(std::move(blend));
        }
}

std::unique_ptr<VulkanKinematics> VulkanKinematics::create(const SkeletonBones& source,
    IRenderVisual* owner, std::string& error)
{
    if (!owner || source.bones.empty() || source.bones.size() > 64 || source.root >= source.bones.size())
    {
        error = "Vulkan skeleton needs a visual and a valid bone table";
        return nullptr;
    }
    auto data = std::make_shared<Data>();
    data->root = source.root;
    if (!source.user_data.empty())
    {
        IReader reader(const_cast<uint8_t*>(source.user_data.data()), source.user_data.size());
        data->user_data = std::make_unique<CInifile>(&reader, FS.get_path("$game_config$") ? FS.get_path("$game_config$")->m_Path : nullptr);
    }
    data->owned.reserve(source.bones.size());
    data->bones.reserve(source.bones.size());
    for (u16 i = 0; i < source.bones.size(); ++i)
    {
        const auto& source_bone = source.bones[i];
        auto bone = std::make_unique<CBoneData>(i);
        bone->name = source_bone.name.c_str();
        bone->SetParentID(source_bone.parent);
        static_assert(sizeof(Fobb) == 60, "OGF bone OBB layout changed");
        std::memcpy(&bone->obb, source_bone.obb.data(), sizeof(Fobb));
        bone->bind_transform.identity();
        bone->m2b_transform.identity();
        bone->shape.Reset();
        bone->IK_data.Reset();
        bone->game_mtl_name = "default_object";
        bone->game_mtl_idx = u16(-1);
        bone->mass = 0.f;
        bone->center_of_mass.set(0.f, 0.f, 0.f);
        if (!source_bone.ik_data.empty())
        {
            IReader reader(const_cast<uint8_t*>(source_bone.ik_data.data()), source_bone.ik_data.size());
            const u16 version = static_cast<u16>(reader.r_u32());
            reader.r_stringZ(bone->game_mtl_name);
            reader.r(&bone->shape, sizeof(SBoneShape));
            bone->IK_data.Import(reader, version);
            Fvector rotation, translation;
            reader.r_fvector3(rotation);
            reader.r_fvector3(translation);
            bone->bind_transform.setXYZi(rotation);
            bone->bind_transform.translate_over(translation);
            bone->mass = reader.r_float();
            reader.r_fvector3(bone->center_of_mass);
        }
        data->names.emplace_back(bone->name, i);
        data->bones.push_back(bone.get());
        data->owned.push_back(std::move(bone));
    }
    std::sort(data->names.begin(), data->names.end(), [](const auto& a, const auto& b)
    { return xr_strcmp(a.first, b.first) < 0; });
    for (auto* bone : data->bones)
        if (bone->GetParentID() != BI_NONE)
            data->bones[bone->GetParentID()]->children.push_back(bone);
    data->bones[source.root]->CalculateM2B(Fidentity);
    auto result = std::unique_ptr<VulkanKinematics>(new VulkanKinematics(std::move(data), owner));
    result->root_ = source.root;
    result->CalculateBones(TRUE);
    error.clear();
    return result;
}

bool VulkanKinematics::attach_geometry(const ModelGeometry& geometry, std::string& error)
{
    if (!data_->meshes.empty() || (geometry.type != 3 && geometry.type != 10))
    {
        error = "Vulkan skeleton has invalid or duplicate child geometry";
        return false;
    }
    std::vector<ModelGeometry> meshes = geometry.children;
    bool has_skinned = false;
    for (const auto& child : meshes)
    {
        if (child.type != 4 && child.type != 5)
            continue;
        has_skinned = true;
        if (child.indices.size() % 3)
        {
            error = "Vulkan skeletal child has incomplete triangles";
            return false;
        }
    }
    if (!has_skinned)
    {
        error = "Vulkan skeleton has no skinned child geometry";
        return false;
    }
    for (auto* bone : data_->bones)
        bone->child_faces.resize(meshes.size());
    for (size_t child = 0; child < meshes.size(); ++child)
    {
        const auto& mesh = meshes[child];
        if (mesh.type != 4 && mesh.type != 5)
            continue;
        if (mesh.indices.size() / 3 > UINT16_MAX)
        {
            error = "Vulkan skeletal child exceeds the bone face index range";
            for (auto* bone : data_->bones)
                bone->child_faces.clear();
            return false;
        }
        for (size_t face = 0; face < mesh.indices.size() / 3; ++face)
        {
            u64 mask = 0;
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = mesh.vertices[mesh.indices[face * 3 + corner]];
                for (size_t link = 0; link < 4; ++link)
                    if (vertex.weights[link] > 0.f)
                    {
                        if (vertex.bones[link] >= data_->bones.size())
                        {
                            error = "Vulkan skeletal child references an unknown bone";
                            for (auto* bone : data_->bones)
                                bone->child_faces.clear();
                            return false;
                        }
                        mask |= u64(1) << vertex.bones[link];
                    }
            }
            for (size_t bone = 0; bone < data_->bones.size(); ++bone)
                if (mask & (u64(1) << bone))
                    data_->bones[bone]->child_faces[child].push_back(static_cast<u16>(face));
        }
    }
    data_->meshes = std::move(meshes);
    error.clear();
    return true;
}

void VulkanKinematics::reset_instance_state()
{
    for (auto& instance : instances_)
        instance.construct();
    offsets_.clear();
    playback_.reset();
    blends_.clear();
    last_motion_frame_ = UINT32_MAX;
    root_ = data_->root;
    visible_ = instances_.size() == 64 ? ~u64(0) : (u64(1) << instances_.size()) - 1;
    update_ = nullptr;
    update_param_ = nullptr;
    dirty_ = true;
    CalculateBones(TRUE);
}

void VulkanKinematics::Bone_Calculate(CBoneData* bone, Fmatrix* parent)
{
    if (!bone || !parent) return;
    auto& instance = LL_GetBoneInstance(bone->GetSelfID());
    if (LL_GetBoneVisible(bone->GetSelfID()))
    {
        if (instance.callback_overwrite())
        {
            if (instance.callback())
                instance.callback()(&instance);
        }
        else
        {
            MotionKey key;
            if (playback_.sample(bone->GetSelfID(), key))
            {
                Fquaternion rotation;
                rotation.x = key.rotation[0];
                rotation.y = key.rotation[1];
                rotation.z = key.rotation[2];
                rotation.w = key.rotation[3];
                Fvector translation;
                translation.set(key.translation[0], key.translation[1], key.translation[2]);
                Fmatrix local;
                local.mk_xform(rotation, translation);
                instance.mTransform.mul_43(*parent, local);
            }
            else instance.mTransform.mul_43(*parent, bone->bind_transform);
            for (const auto& offset : offsets_)
                if (offset.m_bone_id == bone->GetSelfID())
                    instance.mTransform.mulB_43(offset.m_transform);
            if (instance.callback())
                instance.callback()(&instance);
        }
        instance.mRenderTransform.mul_43(instance.mTransform, bone->m2b_transform);
    }
    for (auto* child : bone->children)
        Bone_Calculate(child, &instance.mTransform);
}

void VulkanKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8, bool)
{
    CalculateBones();
    pos = LL_GetTransform(id);
}

bool VulkanKinematics::PickBone(const Fmatrix& parent, pick_result& result, float range, const Fvector& start, const Fvector& direction, u16 id)
{
    R_ASSERT2(id < data_->bones.size(), "invalid Vulkan pick bone ID");
    CalculateBones();
    Fmatrix inverse;
    inverse.invert(parent);
    Fvector origin, ray;
    inverse.transform_tiny(origin, start);
    inverse.transform_dir(ray, direction);
    bool hit = false;
    for (size_t child = 0; child < data_->meshes.size(); ++child)
    {
        const auto& mesh = data_->meshes[child];
        if (mesh.type != 4 && mesh.type != 5)
            continue;
        std::vector<float> pose(instances_.size() * 16);
        for (size_t bone = 0; bone < instances_.size(); ++bone)
            std::memcpy(pose.data() + bone * 16, &instances_[bone].mRenderTransform, sizeof(Fmatrix));
        std::vector<LevelVertex> vertices;
        std::string error;
        if (!skin_model_mesh(mesh, pose.data(), instances_.size(), vertices, error))
            continue;
        for (u16 face : data_->bones[id]->child_faces[child])
        {
            Fvector tri[3];
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = vertices[mesh.indices[size_t(face) * 3 + corner]];
                tri[corner].set(vertex.position[0], vertex.position[1], vertex.position[2]);
            }
            Fvector edge1, edge2, cross, from;
            edge1.sub(tri[1], tri[0]);
            edge2.sub(tri[2], tri[0]);
            cross.crossproduct(ray, edge2);
            const float determinant = edge1.dotproduct(cross);
            if (std::abs(determinant) < 0.000001f)
                continue;
            const float inv_det = 1.f / determinant;
            from.sub(origin, tri[0]);
            const float u = from.dotproduct(cross) * inv_det;
            if (u < 0.f || u > 1.f)
                continue;
            Fvector q;
            q.crossproduct(from, edge1);
            const float v = ray.dotproduct(q) * inv_det;
            if (v < 0.f || u + v > 1.f)
                continue;
            const float distance = edge2.dotproduct(q) * inv_det;
            if (distance < 0.f || distance >= range)
                continue;
            range = distance;
            result.dist = distance;
            for (size_t corner = 0; corner < 3; ++corner)
                parent.transform_tiny(result.tri[corner], tri[corner]);
            result.normal.mknormal(result.tri[0], result.tri[1], result.tri[2]);
            hit = true;
        }
    }
    return hit;
}

void VulkanKinematics::EnumBoneVertices(SEnumVerticesCallback& callback, u16 id)
{
    R_ASSERT2(id < data_->bones.size(), "invalid Vulkan bone enumeration ID");
    CalculateBones();
    std::vector<float> pose(instances_.size() * 16);
    for (size_t bone = 0; bone < instances_.size(); ++bone)
        std::memcpy(pose.data() + bone * 16, &instances_[bone].mRenderTransform, sizeof(Fmatrix));
    for (size_t child = 0; child < data_->meshes.size(); ++child)
    {
        const auto& mesh = data_->meshes[child];
        if (mesh.type != 4 && mesh.type != 5) continue;
        std::vector<LevelVertex> vertices;
        std::string error;
        if (!skin_model_mesh(mesh, pose.data(), instances_.size(), vertices, error)) continue;
        for (u16 face : data_->bones[id]->child_faces[child])
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = vertices[mesh.indices[size_t(face) * 3 + corner]];
                Fvector position;
                position.set(vertex.position[0], vertex.position[1], vertex.position[2]);
                callback(position);
            }
    }
}

CInifile* VulkanKinematics::LL_UserData()
{
    return data_->user_data.get();
}

u16 VulkanKinematics::LL_BoneID(LPCSTR name)
{
    if (name)
        for (const auto& bone : data_->names)
            if (xr_stricmp(bone.first.c_str(), name) == 0) return bone.second;
    return BI_NONE;
}

u16 VulkanKinematics::LL_BoneID(const shared_str& name) { return LL_BoneID(name.c_str()); }
LPCSTR VulkanKinematics::LL_BoneName_dbg(u16 id) { return LL_GetData(id).name.c_str(); }
IKinematics::accel* VulkanKinematics::LL_Bones() { return &data_->names; }

CBoneInstance& VulkanKinematics::LL_GetBoneInstance(u16 id)
{
    R_ASSERT2(id < instances_.size(), "invalid Vulkan bone instance ID");
    return instances_[id];
}

CBoneData& VulkanKinematics::LL_GetData(u16 id)
{
    R_ASSERT2(id < data_->bones.size(), "invalid Vulkan bone data ID");
    return *data_->bones[id];
}

const IBoneData& VulkanKinematics::GetBoneData(u16 id) const
{
    R_ASSERT2(id < data_->bones.size(), "invalid Vulkan bone data ID");
    return *data_->bones[id];
}

u16 VulkanKinematics::LL_BoneCount() const { return static_cast<u16>(instances_.size()); }
u16 VulkanKinematics::LL_VisibleBoneCount()
{
    u16 count = 0;
    for (u64 bits = visible_; bits; bits &= bits - 1) ++count;
    return count;
}
Fmatrix& VulkanKinematics::LL_GetTransform(u16 id) { return LL_GetBoneInstance(id).mTransform; }
const Fmatrix& VulkanKinematics::LL_GetTransform(u16 id) const
{
    R_ASSERT2(id < instances_.size(), "invalid Vulkan bone transform ID");
    return instances_[id].mTransform;
}
Fmatrix& VulkanKinematics::LL_GetTransform_R(u16 id) { return LL_GetBoneInstance(id).mRenderTransform; }
Fobb& VulkanKinematics::LL_GetBox(u16 id) { return LL_GetData(id).obb; }
const Fbox& VulkanKinematics::GetBox() const { return owner_->getVisData().box; }

void VulkanKinematics::bind_transform(u16 id, const Fmatrix& parent, xr_vector<Fmatrix>& matrices)
{
    auto* bone = data_->bones[id];
    matrices[id].mul_43(parent, bone->bind_transform);
    for (auto* child : bone->children)
        bind_transform(child->GetSelfID(), matrices[id], matrices);
}

void VulkanKinematics::LL_GetBindTransform(xr_vector<Fmatrix>& matrices)
{
    matrices.resize(instances_.size());
    bind_transform(root_, Fidentity, matrices);
}

int VulkanKinematics::LL_GetBoneGroups(xr_vector<xr_vector<u16>>& groups)
{
    groups.clear();
    groups.resize(data_->meshes.size());
    for (size_t child = 0; child < groups.size(); ++child)
        for (u16 id = 0; id < data_->bones.size(); ++id)
            if (!data_->bones[id]->child_faces[child].empty())
                groups[child].push_back(id);
    return static_cast<int>(groups.size());
}

void VulkanKinematics::LL_SetBoneRoot(u16 id)
{
    R_ASSERT2(id < instances_.size(), "invalid Vulkan skeleton root");
    root_ = id;
    dirty_ = true;
}

BOOL VulkanKinematics::LL_GetBoneVisible(u16 id)
{
    R_ASSERT2(id < instances_.size(), "invalid Vulkan bone visibility ID");
    return (visible_ & (u64(1) << id)) != 0;
}

void VulkanKinematics::set_visible(u16 id, BOOL visible, BOOL recursive)
{
    const u64 flag = u64(1) << id;
    if (visible) visible_ |= flag;
    else visible_ &= ~flag;
    if (recursive)
        for (auto* child : data_->bones[id]->children)
            set_visible(child->GetSelfID(), visible, TRUE);
}

void VulkanKinematics::LL_SetBoneVisible(u16 id, BOOL visible, BOOL recursive)
{
    R_ASSERT2(id < instances_.size(), "invalid Vulkan bone visibility ID");
    set_visible(id, visible, recursive);
    dirty_ = true;
}

void VulkanKinematics::LL_SetBonesVisible(u64 mask)
{
    visible_ = mask & (instances_.size() == 64 ? ~u64(0) : (u64(1) << instances_.size()) - 1);
    dirty_ = true;
}

void VulkanKinematics::LL_AddTransformToBone(KinematicsABT::additional_bone_transform& transform)
{
    R_ASSERT2(transform.m_bone_id < instances_.size(), "invalid Vulkan bone offset ID");
    offsets_.push_back(transform);
    dirty_ = true;
}

void VulkanKinematics::LL_ClearAdditionalTransform(u16 id)
{
    offsets_.erase(std::remove_if(offsets_.begin(), offsets_.end(), [id](const auto& transform)
    { return id == BI_NONE || transform.m_bone_id == id; }), offsets_.end());
    dirty_ = true;
}

void VulkanKinematics::CalculateBones(BOOL force)
{
    if (playback_.active_count()) UpdateTracks();
    if (!dirty_ && !force) return;
    Fmatrix identity = Fidentity;
    Bone_Calculate(data_->bones[root_], &identity);
    dirty_ = false;
    Fbox box;
    box.invalidate();
    bool has_visible = false;
    for (size_t index = 0; index < instances_.size(); ++index)
    {
        if (!LL_GetBoneVisible(static_cast<u16>(index)))
            continue;
        Fobb& obb = data_->bones[index]->obb;
        Fmatrix local, transformed;
        obb.xform_get(local);
        transformed.mul_43(instances_[index].mTransform, local);
        for (int sx : { -1, 1 })
            for (int sy : { -1, 1 })
                for (int sz : { -1, 1 })
                {
                    Fvector corner, point;
                    corner.set(sx * obb.m_halfsize.x, sy * obb.m_halfsize.y, sz * obb.m_halfsize.z);
                    transformed.transform_tiny(point, corner);
                    box.modify(point);
                }
        has_visible = true;
    }
    if (has_visible)
    {
        auto& visibility = owner_->getVisData();
        visibility.box = box;
        box.getsphere(visibility.sphere.P, visibility.sphere.R);
    }
    if (update_) update_(this);
}

void VulkanKinematics::Callback(UpdateCallback callback, void* param)
{
    update_ = callback;
    update_param_ = param;
}
}
#include "xrEngine/stdafx.h"
#include "VulkanKinematics.h"
#include "xrEngine/device.h"

#include <algorithm>
#include <cmath>

namespace xray::render::vulkan
{
bool VulkanKinematics::valid_motion(MotionID id) const
{
    return id.valid() && id.slot < data_->motions.size() &&
        id.idx < data_->motions[id.slot].definitions.size() &&
        data_->motions[id.slot].definitions[id.idx].motion < data_->motions[id.slot].clips.size();
}

u16 VulkanKinematics::LL_MotionsSlotCount()
{
    return static_cast<u16>(data_->motions.size());
}

const shared_motions& VulkanKinematics::LL_MotionsSlot(u16 index)
{
    R_ASSERT2(index < data_->legacy_motions.size(), "invalid Vulkan motion slot");
    return data_->legacy_motions[index];
}

CMotionDef* VulkanKinematics::LL_GetMotionDef(MotionID id)
{
    if (!valid_motion(id) || id.slot >= data_->legacy_motions.size()) return nullptr;
    return data_->legacy_motions[id.slot].motion_def(id.idx);
}

CMotion* VulkanKinematics::LL_GetMotion(MotionID id, u16 bone)
{
    if (!valid_motion(id) || bone >= LL_BoneCount() || id.slot >= data_->legacy_motions.size()) return nullptr;
    MotionVec* tracks = data_->legacy_motions[id.slot].bone_motions(LL_GetData(bone).name);
    const auto* def = LL_GetMotionDef(id);
    return tracks && def && def->motion < tracks->size() ? &(*tracks)[def->motion] : nullptr;
}

CMotion* VulkanKinematics::LL_GetRootMotion(MotionID id)
{
    return LL_GetMotion(id, root_);
}

MotionID VulkanKinematics::LL_MotionID(LPCSTR name)
{
    if (!name) return {};
    for (size_t slot = data_->motions.size(); slot > 0; --slot)
        for (size_t index = 0; index < data_->motions[slot - 1].definitions.size(); ++index)
            if (xr_stricmp(data_->motions[slot - 1].definitions[index].name.c_str(), name) == 0)
                return MotionID(static_cast<u16>(slot - 1), static_cast<u16>(index));
    return {};
}

u16 VulkanKinematics::LL_PartID(LPCSTR name)
{
    if (name && !data_->motions.empty())
        for (size_t index = 0; index < data_->motions[0].partition_names.size(); ++index)
            if (xr_stricmp(data_->motions[0].partition_names[index].c_str(), name) == 0)
                return static_cast<u16>(index);
    return BI_NONE;
}

MotionID VulkanKinematics::ID_Cycle_Safe(LPCSTR name)
{
    if (!name) return {};
    for (size_t slot = data_->motions.size(); slot > 0; --slot)
        for (size_t index = 0; index < data_->motions[slot - 1].definitions.size(); ++index)
        {
            const auto& def = data_->motions[slot - 1].definitions[index];
            if (!(def.flags & 1) && xr_stricmp(def.name.c_str(), name) == 0)
                return MotionID(static_cast<u16>(slot - 1), static_cast<u16>(index));
        }
    return {};
}
MotionID VulkanKinematics::ID_Cycle(LPCSTR name)
{
    MotionID id = ID_Cycle_Safe(name);
    R_ASSERT2(id.valid(), "Vulkan skeleton has no requested cycle");
    return id;
}
MotionID VulkanKinematics::ID_Cycle(shared_str name) { return ID_Cycle(name.c_str()); }
MotionID VulkanKinematics::ID_Cycle_Safe(shared_str name) { return ID_Cycle_Safe(name.c_str()); }
MotionID VulkanKinematics::ID_FX_Safe(LPCSTR name)
{
    if (!name) return {};
    for (size_t slot = data_->motions.size(); slot > 0; --slot)
        for (size_t index = 0; index < data_->motions[slot - 1].definitions.size(); ++index)
        {
            const auto& def = data_->motions[slot - 1].definitions[index];
            if ((def.flags & 1) && xr_stricmp(def.name.c_str(), name) == 0)
                return MotionID(static_cast<u16>(slot - 1), static_cast<u16>(index));
        }
    return {};
}
MotionID VulkanKinematics::ID_FX(LPCSTR name)
{
    MotionID id = ID_FX_Safe(name);
    R_ASSERT2(id.valid(), "Vulkan skeleton has no requested FX");
    return id;
}

CBlend* VulkanKinematics::create_blend(u16 part, MotionID id, bool fx, bool mixing,
    float accrue, float falloff, float speed, float power, bool noloop,
    PlayCallback callback, LPVOID param, u8 channel)
{
    if (!valid_motion(id) || channel >= 4 || !std::isfinite(power) || power < 0.f ||
        blends_.size() >= MAX_BLENDED_POOL) return nullptr;
    const auto& definition = data_->motions[id.slot].definitions[id.idx];
    if (bool(definition.flags & 1) != fx) return nullptr;
    if (!fx && part != BI_NONE && part >= data_->motions[0].partitions.size()) return nullptr;
    if (fx && part == BI_NONE) part = root_;
    auto blend = std::make_unique<CBlend>();
    CBlend* result = blend.get();
    result->motionID = id;
    result->bone_or_part = part;
    result->channel = channel;
    result->blendAccrue = accrue;
    result->blendFalloff = falloff;
    result->blendPower = power * definition.parameters[1];
    result->speed = speed;
    result->timeCurrent = 0.f;
    result->timeTotal = get_animation_length(id);
    result->blendAmount = mixing ? 0.f : result->blendPower;
    result->playing = true;
    result->stop_at_end = fx || noloop;
    result->stop_at_end_callback = true;
    result->fall_at_end = fx;
    result->Callback = callback;
    result->CallbackParam = param;
    result->dwFrame = UINT32_MAX;
    result->set_accrue_state();
    MotionPlayback::Handle handle{id.slot, id.idx};
    if (!playback_.play(handle, fx, mixing, power,
            [callback, result] { if (callback) callback(result); }, accrue, falloff, speed, noloop, channel, part))
        return nullptr;
    if (!fx && !mixing)
        for (auto& old : blends_)
            if (old->blend_state() != CBlend::eFREE_SLOT && old->channel == channel &&
                old->bone_or_part == part) old->set_falloff_state();
    blends_.push_back(std::move(blend));
    dirty_ = true;
    return result;
}

CBlend* VulkanKinematics::LL_PlayCycle(u16 part, MotionID id, BOOL mixing, float accrue,
    float falloff, float speed, BOOL noloop, PlayCallback callback, LPVOID param, u8 channel)
{
    if (!valid_motion(id)) return nullptr;
    if (part == BI_NONE)
    {
        CBlend* result = nullptr;
        for (u16 index = 0; index < data_->motions[0].partitions.size(); ++index)
            result = LL_PlayCycle(index, id, mixing, accrue, falloff, speed, noloop, callback, param, channel);
        return result;
    }
    return create_blend(part, id, false, mixing, accrue, falloff, speed, 1.f,
        noloop, callback, param, channel);
}

CBlend* VulkanKinematics::LL_PlayCycle(u16 part, MotionID id, BOOL mixing,
    PlayCallback callback, LPVOID param, u8 channel)
{
    if (!valid_motion(id)) return nullptr;
    const auto& def = data_->motions[id.slot].definitions[id.idx];
    return LL_PlayCycle(part, id, mixing, def.parameters[2], def.parameters[3],
        def.parameters[0], (def.flags & 2) != 0, callback, param, channel);
}

CBlend* VulkanKinematics::PlayCycle(LPCSTR name, BOOL mixing,
    PlayCallback callback, LPVOID param, u8 channel)
{
    return PlayCycle(ID_Cycle_Safe(name), mixing, callback, param, channel);
}
CBlend* VulkanKinematics::PlayCycle(MotionID id, BOOL mixing,
    PlayCallback callback, LPVOID param, u8 channel)
{
    if (!valid_motion(id)) return nullptr;
    return LL_PlayCycle(data_->motions[id.slot].definitions[id.idx].bone_or_part,
        id, mixing, callback, param, channel);
}
CBlend* VulkanKinematics::PlayCycle(u16 part, MotionID id, BOOL mixing,
    PlayCallback callback, LPVOID param, u8 channel)
{
    return LL_PlayCycle(part, id, mixing, callback, param, channel);
}

CBlend* VulkanKinematics::PlayFX(MotionID id, float power)
{
    if (!valid_motion(id)) return nullptr;
    const auto& def = data_->motions[id.slot].definitions[id.idx];
    return create_blend(def.bone_or_part, id, true, true, def.parameters[2],
        def.parameters[3], def.parameters[0], power,
        true, nullptr, nullptr, 0);
}
CBlend* VulkanKinematics::PlayFX(LPCSTR name, float power) { return PlayFX(ID_FX_Safe(name), power); }
CBlend* VulkanKinematics::PlayFX_Safe(cpcstr name, float power) { return PlayFX(ID_FX_Safe(name), power); }

void VulkanKinematics::LL_CloseCycle(u16 part, u8 mask)
{
    playback_.stop_cycles(part, mask);
    for (auto& blend : blends_)
        if (blend->blend_state() != CBlend::eFREE_SLOT && blend->channel < 4 &&
            (mask & (1u << blend->channel)) &&
            (part == BI_NONE || part == blend->bone_or_part) &&
            !(data_->motions[blend->motionID.slot].definitions[blend->motionID.idx].flags & 1))
            blend->set_falloff_state();
    dirty_ = true;
}

void VulkanKinematics::LL_SetChannelFactor(u16 channel, float factor)
{
    if (channel < 4 && std::isfinite(factor))
    {
        channel_factors_[channel] = std::clamp(factor, 0.f, 1.f);
        playback_.set_channel_factor(static_cast<u8>(channel), factor);
        dirty_ = true;
    }
}

void VulkanKinematics::LL_UpdateTracks(float dt, bool, bool leave_blends)
{
    if (tracks_update_ && tracks_update_->operator()(dt, *this)) return;
    playback_.advance(dt);
    for (auto& blend : blends_)
    {
        if (blend->blend_state() == CBlend::eFREE_SLOT) continue;
        blend->timeCurrent = std::min(blend->timeCurrent + dt * blend->speed, blend->timeTotal);
        if (blend->blend_state() == CBlend::eFalloff)
            blend->blendAmount = std::max(0.f, blend->blendAmount - dt * blend->blendFalloff * blend->blendPower);
        else
            blend->blendAmount = std::min(blend->blendPower, blend->blendAmount + dt * blend->blendAccrue * blend->blendPower);
        if (blend->timeCurrent >= blend->timeTotal && blend->stop_at_end)
            blend->set_falloff_state();
        if (!leave_blends && blend->blendAmount <= 0.f && blend->blend_state() == CBlend::eFalloff)
        {
            if (blend_destroy_) blend_destroy_->BlendDestroy(*blend);
            blend->set_free_state();
        }
    }
    dirty_ = true;
}

void VulkanKinematics::UpdateTracks()
{
    if (last_motion_frame_ == Device.dwFrame) return;
    last_motion_frame_ = Device.dwFrame;
    LL_UpdateTracks(Device.fTimeDelta, false, false);
}

u32 VulkanKinematics::LL_PartBlendsCount(u32 part)
{
    u32 count = 0;
    for (const auto& blend : blends_)
        if (blend->blend_state() != CBlend::eFREE_SLOT && blend->bone_or_part == part &&
            !(data_->motions[blend->motionID.slot].definitions[blend->motionID.idx].flags & 1)) ++count;
    return count;
}
CBlend* VulkanKinematics::LL_PartBlend(u32 part, u32 index)
{
    for (const auto& blend : blends_)
        if (blend->blend_state() != CBlend::eFREE_SLOT && blend->bone_or_part == part &&
            !(data_->motions[blend->motionID.slot].definitions[blend->motionID.idx].flags & 1))
        {
            if (!index--) return blend.get();
        }
    return nullptr;
}
void VulkanKinematics::LL_IterateBlends(IterateBlendsCallback& callback)
{
    for (const auto& blend : blends_)
        if (blend->blend_state() != CBlend::eFREE_SLOT) callback(*blend);
}

void VulkanKinematics::LL_BuldBoneMatrixDequatize(const CBoneData* bone, u8 mask, SKeyTable& keys)
{
    if (!bone || !(mask & 1)) return;
    MotionKey key;
    if (!playback_.sample(bone->GetSelfID(), key)) return;
    CKey& out = keys.keys[0][0];
    out.Q.x = key.rotation[0]; out.Q.y = key.rotation[1];
    out.Q.z = key.rotation[2]; out.Q.w = key.rotation[3];
    out.T.set(key.translation[0], key.translation[1], key.translation[2]);
    keys.chanel_blend_conts[0] = 1;
    keys.blends[0][0] = nullptr;
}
void VulkanKinematics::LL_BoneMatrixBuild(CBoneInstance& instance, const Fmatrix* parent, const SKeyTable& keys)
{
    R_ASSERT2(parent, "Vulkan bone matrix requires a parent");
    if (!keys.chanel_blend_conts[0]) { instance.mTransform = *parent; return; }
    Fmatrix local;
    local.mk_xform(keys.keys[0][0].Q, keys.keys[0][0].T);
    instance.mTransform.mul_43(*parent, local);
}

float VulkanKinematics::get_animation_length(MotionID id)
{
    if (!valid_motion(id)) return 0.f;
    const auto& slot = data_->motions[id.slot];
    return slot.clips[slot.definitions[id.idx].motion].frames / 30.f;
}

#ifdef DEBUG
std::pair<LPCSTR, LPCSTR> VulkanKinematics::LL_MotionDefName_dbg(MotionID id)
{
    if (!valid_motion(id)) return {nullptr, nullptr};
    return {data_->motions[id.slot].definitions[id.idx].name.c_str(), data_->motions[id.slot].source.c_str()};
}
void VulkanKinematics::LL_DumpBlends_dbg()
{
    Msg("* Vulkan skeleton: %zu active blend slots", blends_.size());
}
#endif
} // namespace xray::render::vulkan
