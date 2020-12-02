#include "stdafx.h"
#pragma hdrstop

#include "Bone.hpp"
#include "xrCore/xrDebug_macros.h"
#include "xrCommon/math_funcs_inline.h"

#define BONE_VERSION 0x0002
//------------------------------------------------------------------------------
#define BONE_CHUNK_VERSION 0x0001
#define BONE_CHUNK_DEF 0x0002
#define BONE_CHUNK_BIND_POSE 0x0003
#define BONE_CHUNK_MATERIAL 0x0004
#define BONE_CHUNK_SHAPE 0x0005
#define BONE_CHUNK_IK_JOINT 0x0006
#define BONE_CHUNK_MASS 0x0007
#define BONE_CHUNK_FLAGS 0x0008
#define BONE_CHUNK_IK_JOINT_BREAK 0x0009
#define BONE_CHUNK_IK_JOINT_FRICTION 0x0010

static const Fobb dummy = Fobb().identity();

const Fobb& CBone::get_obb() const { return dummy; }

bool SBoneShape::Valid() const
{
    switch (type)
    {
    case stBox:
        return !fis_zero(box.m_halfsize.x) && !fis_zero(box.m_halfsize.y) && !fis_zero(box.m_halfsize.z);
    case stSphere: return !fis_zero(sphere.R);
    case stCylinder:
        return !fis_zero(cylinder.m_height) && !fis_zero(cylinder.m_radius) &&
            !fis_zero(cylinder.m_direction.square_magnitude());
    default:
        return true;
    }
}

void SJointIKData::Export(IWriter& F)
{
    F.w_u32(type);
    for (SJointLimit& joint_limit : limits)
    {
        // Kostya Slipchenko say:
        // направление вращения в ОДЕ отличается от направления вращение в X-Ray
        // поэтому меняем знак у лимитов
        // F.w_float (std::min(-joint_limit.limit.x,-joint_limit.limit.y)); // min (swap special for ODE)
        // F.w_float (std::max(-joint_limit.limit.x,-joint_limit.limit.y)); // max (swap special for ODE)

        VERIFY(std::min(-joint_limit.limit.x, -joint_limit.limit.y) == -joint_limit.limit.y);
        VERIFY(std::max(-joint_limit.limit.x, -joint_limit.limit.y) == -joint_limit.limit.x);

        F.w_float(-joint_limit.limit.y); // min (swap special for ODE)
        F.w_float(-joint_limit.limit.x); // max (swap special for ODE)

        F.w_float(joint_limit.spring_factor);
        F.w_float(joint_limit.damping_factor);
    }
    F.w_float(spring_factor);
    F.w_float(damping_factor);

    F.w_u32(ik_flags.get());
    F.w_float(break_force);
    F.w_float(break_torque);

    F.w_float(friction);
}

bool SJointIKData::Import(IReader& F, u16 vers)
{
    type = (EJointType)F.r_u32();
    F.r(limits, sizeof(SJointLimit) * 3);
    spring_factor = F.r_float();
    damping_factor = F.r_float();
    ik_flags.flags = F.r_u32();
    break_force = F.r_float();
    break_torque = F.r_float();

    if (vers > 0)
        friction = F.r_float();

    return true;
}

CBone::CBone()
{
    construct();
    flags.zero();
    rest_length = 0;
    SelfID = -1;
    parent = nullptr;

    ResetData();
}

void CBone::ResetData()
{
    IK_data.Reset();
    game_mtl = "default_object";
    shape.Reset();

    mass = 10.f;
    center_of_mass.set(0.f, 0.f, 0.f);
}

void CBone::Save(IWriter& F)
{
    F.open_chunk(BONE_CHUNK_VERSION);
    F.w_u16(BONE_VERSION);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_DEF);
    F.w_stringZ(name);
    F.w_stringZ(parent_name);
    F.w_stringZ(wmap);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_BIND_POSE);
    F.w_fvector3(rest_offset);
    F.w_fvector3(rest_rotate);
    F.w_float(rest_length);
    F.close_chunk();

    SaveData(F);
}

[[maybe_unused]] void CBone::Load_0(IReader& F)
{
    F.r_stringZ(name);
    xr_strlwr(name);
    F.r_stringZ(parent_name);
    xr_strlwr(parent_name);
    F.r_stringZ(wmap);
    F.r_fvector3(rest_offset);
    F.r_fvector3(rest_rotate);
    rest_length = F.r_float();
    std::swap(rest_rotate.x, rest_rotate.y);
    Reset();
}

[[maybe_unused]] void CBone::Load_1(IReader& F)
{
    R_ASSERT(F.find_chunk(BONE_CHUNK_VERSION));
    u16 ver = F.r_u16();

    if ((ver != 0x0001) && (ver != BONE_VERSION))
        return;

    R_ASSERT(F.find_chunk(BONE_CHUNK_DEF));
    F.r_stringZ(name);
    xr_strlwr(name);
    F.r_stringZ(parent_name);
    xr_strlwr(parent_name);
    F.r_stringZ(wmap);

    R_ASSERT(F.find_chunk(BONE_CHUNK_BIND_POSE));
    F.r_fvector3(rest_offset);
    F.r_fvector3(rest_rotate);
    rest_length = F.r_float();

    if (ver == 0x0001)
        std::swap(rest_rotate.x, rest_rotate.y);

    LoadData(F);
}

void CBone::SaveData(IWriter& F)
{
    F.open_chunk(BONE_CHUNK_DEF);
    F.w_stringZ(name);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_MATERIAL);
    F.w_stringZ(game_mtl);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_SHAPE);
    F.w(&shape, sizeof(SBoneShape));
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_FLAGS);
    F.w_u32(IK_data.ik_flags.get());
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_IK_JOINT);
    F.w_u32(IK_data.type);
    F.w(IK_data.limits, sizeof(SJointLimit) * 3);
    F.w_float(IK_data.spring_factor);
    F.w_float(IK_data.damping_factor);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_IK_JOINT_BREAK);
    F.w_float(IK_data.break_force);
    F.w_float(IK_data.break_torque);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_IK_JOINT_FRICTION);
    F.w_float(IK_data.friction);
    F.close_chunk();

    F.open_chunk(BONE_CHUNK_MASS);
    F.w_float(mass);
    F.w_fvector3(center_of_mass);
    F.close_chunk();
}

void CBone::LoadData(IReader& F)
{
    R_ASSERT(F.find_chunk(BONE_CHUNK_DEF));
    F.r_stringZ(name);
    xr_strlwr(name);

    R_ASSERT(F.find_chunk(BONE_CHUNK_MATERIAL));
    F.r_stringZ(game_mtl);

    R_ASSERT(F.find_chunk(BONE_CHUNK_SHAPE));
    F.r(&shape, sizeof(SBoneShape));

    if (F.find_chunk(BONE_CHUNK_FLAGS))
        IK_data.ik_flags.assign(F.r_u32());

    R_ASSERT(F.find_chunk(BONE_CHUNK_IK_JOINT));
    IK_data.type = (EJointType)F.r_u32();
    F.r(IK_data.limits, sizeof(SJointLimit) * 3);
    IK_data.spring_factor = F.r_float();
    IK_data.damping_factor = F.r_float();

    if (F.find_chunk(BONE_CHUNK_IK_JOINT_BREAK))
    {
        IK_data.break_force = F.r_float();
        IK_data.break_torque = F.r_float();
    }

    if (F.find_chunk(BONE_CHUNK_IK_JOINT_FRICTION))
    {
        IK_data.friction = F.r_float();
    }

    if (F.find_chunk(BONE_CHUNK_MASS))
    {
        mass = F.r_float();
        F.r_fvector3(center_of_mass);
    }
}

void CBone::CopyData(CBone* bone)
{
    game_mtl = bone->game_mtl;
    shape = bone->shape;
    IK_data = bone->IK_data;
    mass = bone->mass;
    center_of_mass = bone->center_of_mass;
}

void CBoneInstance::set_param(u32 idx, float data)
{
    VERIFY(idx < MAX_BONE_PARAMS);
    param[idx] = data;
}
float CBoneInstance::get_param(u32 idx)
{
    VERIFY(idx < MAX_BONE_PARAMS);
    return param[idx];
}

#ifdef DEBUG
void CBoneData::DebugQuery(BoneDebug& L)
{
    for (CBoneData* c : children)
    {
        L.push_back(SelfID);
        L.push_back(c->SelfID);
        c->DebugQuery(L);
    }
}
#endif

void CBoneData::CalculateM2B(const Fmatrix& parent)
{
    // Build matrix
    m2b_transform.mul_43(parent, bind_transform);

    // Calculate children
    for (CBoneData* c : children)
        c->CalculateM2B(m2b_transform);

    m2b_transform.invert();
}
