#include "StdAfx.h"
#include "bone_groups.h"
#include "Actor.h"
#include "Include/xrRender/Kinematics.h"

namespace award_system
{
bone_group::bone_group() {}
bone_group::~bone_group() {}
void bone_group::init(CActor* actor_ptr)
{
    if (!m_bone_groups.empty())
        return;

    IKinematics* V = smart_cast<IKinematics*>(actor_ptr->Visual());
    VERIFY(V);

    m_bone_groups.insert(std::make_pair(V->LL_BoneID("bip01_head"), gid_head));

    m_bone_groups.insert(std::make_pair(V->LL_BoneID("eye_left"), gid_eyes));
    m_bone_groups.insert(std::make_pair(V->LL_BoneID("eye_right"), gid_eyes));

    m_bone_groups.insert(std::make_pair(V->LL_BoneID("bip01_spine"), gid_spine));
    m_bone_groups.insert(std::make_pair(V->LL_BoneID("bip01_spine1"), gid_spine));
    m_bone_groups.insert(std::make_pair(V->LL_BoneID("bip01_spine2"), gid_spine));
    m_bone_groups.insert(std::make_pair(V->LL_BoneID("bip01_spine2"), gid_spine));
}

bool bone_group::is_bone_in_group(u16 bone_id, enum_group_id gid) const
{
    if (gid == gid_any)
        return true;

    bone_groups_map_t::const_iterator tmp_iter = m_bone_groups.find(bone_id);
    if (tmp_iter == m_bone_groups.end())
        return false;

    return (tmp_iter->second == gid);
}

} // namespace award_system
