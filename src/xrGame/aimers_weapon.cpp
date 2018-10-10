////////////////////////////////////////////////////////////////////////////
//	Module 		: aimers_weapon.cpp
//	Created 	: 04.04.2008
//  Modified 	: 04.04.2008
//	Author		: Dmitriy Iassenev
//	Description : weapon aimer class
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "aimers_weapon.h"
#include "Include/xrRender/Kinematics.h"
#include "animation_movement_controller.h"
#include "game_object_space.h"
#include "GameObject.h"
#include "animation_movement_controller.h"
#include "Weapon.h"

using aimers::weapon;

weapon::weapon(CGameObject* object, LPCSTR animation_id, bool animation_start, Fvector const& target, LPCSTR bone0,
    LPCSTR bone1, LPCSTR weapon_bone0, LPCSTR weapon_bone1, CWeapon const& weapon)
    : inherited(object, animation_id, animation_start, target), m_weapon(weapon)
{
    LPCSTR bones[4] = {bone0, bone1, weapon_bone0, weapon_bone1};
    for (u32 i = 0; i < 4; ++i)
        m_bones_ids[i] = m_kinematics.LL_BoneID(bones[i]);

    CBoneData const& bone_data = m_kinematics.LL_GetData(m_bones_ids[weapon_bone_id0]);
    m_bones_ids[parent_weapon_bone_id0] = bone_data.GetParentID();
    VERIFY(m_bones_ids[parent_weapon_bone_id0] != BI_NONE);

    compute_bone(bone_id0);

#if 1
    Fmatrix& bone_0 = m_result[bone_id0];
    VERIFY(_valid(bone_0));
    Fvector angles;
    bone_0.getXYZ(angles);
    VERIFY(_valid(angles));
    angles.mul(.5f);
    bone_0.setXYZ(angles);
    VERIFY(_valid(bone_0));

    CBoneInstance& bone = m_kinematics.LL_GetBoneInstance(m_bones_ids[bone_id0]);
    BoneCallback const& old_callback = bone.callback();
    void* old_callback_param = bone.callback_param();
    bone.set_callback(bctCustom, &callback, &bone_0);

    compute_bone(bone_id1);

    bone.set_callback(bctCustom, old_callback, old_callback_param);
#else // #if 0
    m_result[bone_id1] = Fidentity;
#endif // #if 0
}

void weapon::compute_bone(u32 const bone_id)
{
    VERIFY(bone_id < weapon_bone_id0);

    u32 bones_ids[] = {bone_id, weapon_bone_id0, weapon_bone_id1, parent_weapon_bone_id0};
    fill_bones(bones_ids, m_bones_ids, m_local_bones, m_bones);

#pragma todo("Dima to Dima: use function from weapon to get this information")

    Fmatrix const& mL = m_local_bones[weapon_bone_id1];
    Fmatrix const& mR = m_local_bones[weapon_bone_id0];

    Fvector pos, ypr;
    pos = pSettings->r_fvector3(m_weapon.cNameSect(), "position");
    ypr = pSettings->r_fvector3(m_weapon.cNameSect(), "orientation");
    ypr.mul(PI / 180.f);

    Fmatrix offset;
    offset.setHPB(ypr.x, ypr.y, ypr.z);
    offset.translate_over(pos);

    Fmatrix mRes;
    Fvector R, D, N;
    D.sub(mL.c, mR.c);

    float const magnitude = D.magnitude();
    if (!fis_zero(magnitude))
        D.div(magnitude);
    else
        D.set(0.f, 0.f, 1.f);

    R.crossproduct(mR.j, D);

    N.crossproduct(D, R);
    N.normalize();

    mRes.set(R, N, D, mR.c);
    mRes.mulA_43(m_start_transform);

    Fvector vLoadedFirePoint = pSettings->r_fvector3(m_weapon.cNameSect(), "fire_point");
    Fmatrix transform;
    transform.mul(mRes, offset);

    Fvector position;
    transform.transform_tiny(position, vLoadedFirePoint);

    Fvector direction = Fvector().set(0.f, 0.f, 1.f);
    transform.transform_dir(direction);

    VERIFY(_valid(m_bones[bone_id]));
    VERIFY(_valid(position));
    VERIFY(_valid(direction));
    aim_at_position(m_bones[bone_id].c, position, direction, m_result[bone_id]);
    VERIFY(_valid(m_result[bone_id]));
    VERIFY(_valid(m_start_transform));
    VERIFY(_valid(Fmatrix(m_start_transform).invert()));
    m_result[bone_id] = Fmatrix(m_start_transform).invert().mulB_43(m_result[bone_id]).mulB_43(m_start_transform);
}
