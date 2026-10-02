#include "xrEngine/stdafx.h"
#include "VulkanKinematics.h"
#include "xrEngine/vis_common.h"
#include "xrCore/FS.h"

#include <algorithm>
#include <cstring>

namespace xray::render::vulkan
{
struct VulkanKinematics::Data
{
    std::vector<std::unique_ptr<CBoneData>> owned;
    vecBones bones;
    accel names;
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

void VulkanKinematics::Bone_Calculate(CBoneData* bone, Fmatrix* parent)
{
    if (!bone || !parent) return;
    auto& instance = LL_GetBoneInstance(bone->GetSelfID());
    instance.mTransform.mul_43(*parent, bone->bind_transform);
    for (const auto& offset : offsets_)
        if (offset.m_bone_id == bone->GetSelfID()) instance.mTransform.mulB_43(offset.m_transform);
    instance.mRenderTransform.mul_43(instance.mTransform, bone->m2b_transform);
    if (instance.callback()) instance.callback()(&instance);
    for (auto* child : bone->children)
        Bone_Calculate(child, &instance.mTransform);
}

void VulkanKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8, bool)
{
    CalculateBones();
    pos = LL_GetTransform(id);
}

bool VulkanKinematics::PickBone(const Fmatrix&, pick_result&, float, const Fvector&,
    const Fvector&, u16)
{
    // Collision and bone vertex enumeration are implemented with #24.
    return false;
}

void VulkanKinematics::EnumBoneVertices(SEnumVerticesCallback&, u16) {}

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
    return 0; // Child face groups are populated by the #23 skeletal mesh path.
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
    if (update_) update_(this);
}

void VulkanKinematics::Callback(UpdateCallback callback, void* param)
{
    update_ = callback;
    update_param_ = param;
}
}
