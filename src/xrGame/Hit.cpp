#include "StdAfx.h"
#include "alife_space.h"
#include "Hit.h"

#include "xrCore/Animation/Bone.hpp"
#include "xrMessages.h"
#include "Level.h"
#include "xrPhysics/MathUtils.h"
#include "xrEngine/xr_object.h"

SHit::SHit(float powerA, Fvector& dirA, IGameObject* whoA, u16 elementA, Fvector p_in_bone_spaceA, float impulseA,
    ALife::EHitType hit_typeA, float armor_piercingA, bool AimBullet) : Time(0), DestID(0), weaponID(0)
{
    power = powerA;
    dir.set(dirA);
    who = whoA;
    if (whoA)
        whoID = whoA->ID();
    else
        whoID = 0;
    boneID = elementA;
    p_in_bone_space.set(p_in_bone_spaceA);
    impulse = impulseA;

    hit_type = hit_typeA;
    armor_piercing = armor_piercingA;
    PACKET_TYPE = 0;
    BulletID = 0;
    SenderID = 0;
    aim_bullet = AimBullet;
    add_wound = true;
}

SHit::SHit() { invalidate(); }
void SHit::invalidate()
{
    Time = 0;
    PACKET_TYPE = 0;
    DestID = 0;

    power = -phInfinity;
    dir.set(-phInfinity, -phInfinity, -phInfinity);
    who = NULL;
    whoID = 0;
    weaponID = 0;

    boneID = BI_NONE;
    p_in_bone_space.set(-phInfinity, -phInfinity, -phInfinity);

    impulse = -phInfinity;
    hit_type = ALife::eHitTypeMax;

    armor_piercing = 0.0f;
    BulletID = 0;
    SenderID = 0;
    aim_bullet = false;
    add_wound = false;
}

bool SHit::is_valide() const { return hit_type != ALife::eHitTypeMax; }
void SHit::GenHeader(u16 PacketType, u16 ID)
{
    DestID = ID;
    PACKET_TYPE = PacketType;
    Time = Level().timeServer();
};

void SHit::Read_Packet(NET_Packet Packet)
{
    u16 type_dummy;
    Packet.r_begin(type_dummy);
    Packet.r_u32(Time);
    Packet.r_u16(PACKET_TYPE);
    Packet.r_u16(DestID);
    Read_Packet_Cont(Packet);
};

void SHit::Read_Packet_Cont(NET_Packet Packet)
{
    Packet.r_u16(whoID);
    Packet.r_u16(weaponID);
    Packet.r_dir(dir);
    Packet.r_float(power);
    Packet.r_u16(boneID);
    Packet.r_vec3(p_in_bone_space);
    Packet.r_float(impulse);
    if (IsGameTypeSingle())
        aim_bullet = Packet.r_u16() != 0;
    else
        aim_bullet = false;
    hit_type = (ALife::EHitType)Packet.r_u16(); // hit type

    if (hit_type == ALife::eHitTypeFireWound)
    {
        Packet.r_float(armor_piercing);
    }
    if (PACKET_TYPE == GE_HIT_STATISTIC)
    {
        Packet.r_u32(BulletID);
        Packet.r_u32(SenderID);
    }
}

void SHit::Write_Packet_Cont(NET_Packet& Packet)
{
    Packet.w_u16(whoID);
    Packet.w_u16(weaponID);
    Packet.w_dir(dir);
    Packet.w_float(power);
    Packet.w_u16(boneID);
    Packet.w_vec3(p_in_bone_space);
    Packet.w_float(impulse);
    if (IsGameTypeSingle())
        Packet.w_u16(aim_bullet != 0);
    Packet.w_u16(u16(hit_type & 0xffff));
    if (hit_type == ALife::eHitTypeFireWound)
    {
        Packet.w_float(armor_piercing);
    }
    if (PACKET_TYPE == GE_HIT_STATISTIC)
    {
        Packet.w_u32(BulletID);
        Packet.w_u32(SenderID);
    }
}
void SHit::Write_Packet(NET_Packet& Packet)
{
    Packet.w_begin(M_EVENT);
    Packet.w_u32(Time);
    Packet.w_u16(u16(PACKET_TYPE & 0xffff));
    Packet.w_u16(u16(DestID & 0xffff));

    Write_Packet_Cont(Packet);
};

#ifdef DEBUG
void SHit::_dump()
{
    Msg("SHit::_dump()---begin");
    Log("power=", power);
    Log("impulse=", impulse);
    Log("dir=", dir);
    Log("whoID=", whoID);
    Log("weaponID=", weaponID);
    Log("element=", boneID);
    Log("p_in_bone_space=", p_in_bone_space);
    Log("hit_type=", (int)hit_type);
    Log("armor_piercing=", armor_piercing);
    Msg("SHit::_dump()---end");
}
#endif
