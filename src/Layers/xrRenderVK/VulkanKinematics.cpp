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
};

VulkanKinematics::VulkanKinematics(std::shared_ptr<Data> data, IRenderVisual* owner)
    : data_(std::move(data)), owner_(owner), instances_(data_->bones.size())
{
    for (auto& instance : instances_) instance.construct();
    visible_ = instances_.size() == 64 ? ~u64(0) : (u64(1) << instances_.size()) - 1;
}

VulkanKinematics::VulkanKinematics(const VulkanKinematics& source, IRenderVisual* owner)
    : data_(source.data_), owner_(owner), instances_(source.instances_), offsets_(source.offsets_),
      root_(source.root_), visible_(source.visible_), dirty_(source.dirty_)
{
    // Callbacks commonly carry a pointer into the old model instance. Copies
    // start without it and may install their own callback after creation.
    for (auto& instance : instances_) instance.reset_callback();
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
            instance.mTransform.mul_43(*parent, bone->bind_transform);
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
    for (size_t child = 0; child < data_->meshes.size(); ++child)
    {
        const auto& mesh = data_->meshes[child];
        for (u16 face : data_->bones[id]->child_faces[child])
            for (size_t corner = 0; corner < 3; ++corner)
            {
                const auto& vertex = mesh.vertices[mesh.indices[size_t(face) * 3 + corner]];
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
