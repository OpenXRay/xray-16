#include "StdAfx.h"
#include "PHCollisionDamageReceiver.h"
#include "xrPhysics/IPhysicsShellHolder.h"
#include "xrCore/xr_ini.h"
#include "Include/xrRender/Kinematics.h"
#include "xrPhysics/Geometry.h"
#include "xrPhysics/PhysicsShell.h"

#include "xrMessages.h"
#include "CharacterPhysicsSupport.h"
void CPHCollisionDamageReceiver::BoneInsert(u16 id, float k)
{
    R_ASSERT2(FindBone(id) == m_controled_bones.end(), "duplicate bone!");
    m_controled_bones.push_back(SControledBone(id, k));
}
void CPHCollisionDamageReceiver::Init()
{
    CPhysicsShellHolder* sh = PPhysicsShellHolder();
    IKinematics* K = smart_cast<IKinematics*>(sh->Visual());
    CInifile* ini = K->LL_UserData();
    if (ini->section_exist("collision_damage"))
    {
        CInifile::Sect& data = ini->r_section("collision_damage");
        for (auto I = data.Data.cbegin(); I != data.Data.cend(); I++)
        {
            const CInifile::Item& item = *I;
            u16 index = K->LL_BoneID(*item.first);
            R_ASSERT3(index != BI_NONE, "Wrong bone name", *item.first);
            BoneInsert(index, float(atof(*item.second)));
            CODEGeom* og = sh->PPhysicsShell()->get_GeomByID(index);
            // R_ASSERT3(og, "collision damage bone has no physics collision", *item.first);
            if (og)
                og->add_obj_contact_cb(DamageReceiverCollisionCallback);
        }
    }
}

const static float hit_threthhold = 5.f;
void CPHCollisionDamageReceiver::CollisionHit(u16 source_id, u16 bone_id, float power, const Fvector& dir, Fvector& pos)
{
    DAMAGE_BONES_I i = FindBone(bone_id);
    if (i == m_controled_bones.end())
        return;
    power *= i->second;
    if (power < hit_threthhold)
        return;

    NET_Packet P;
    CPhysicsShellHolder* ph = PPhysicsShellHolder();
    SHit HS;

    HS.GenHeader(GE_HIT, ph->ID()); //	ph->u_EventGen(P,GE_HIT,ph->ID());
    HS.whoID = ph->ID(); //	P.w_u16		(ph->ID());
    HS.weaponID = source_id; //	P.w_u16		(source_id);
    HS.dir = dir; //	P.w_dir		(dir);
    HS.power = power; //	P.w_float	(power);
    HS.boneID = s16(bone_id); //	P.w_s16		(s16(bone_id));
    HS.p_in_bone_space = pos; //	P.w_vec3	(pos);
    HS.impulse = 0.f; //	P.w_float	(0.f);
    HS.hit_type = (ALife::eHitTypeStrike); //	P.w_u16		(ALife::eHitTypeStrike);
    HS.Write_Packet(P);

    ph->u_EventSend(P);
}

void CPHCollisionDamageReceiver::Clear()
{
    // IPhysicsShellHolder *sh	=PPhysicsShellHolder	();
    // xr_map<u16,float>::iterator i=m_controled_bones.begin(),e=m_controled_bones.end();
    // for(;e!=i;++i)
    //{
    //	CODEGeom* og= sh->PPhysicsShell()->get_GeomByID(i->first);
    //	if(og)og->set_obj_contact_cb(NULL);
    //}
    m_controled_bones.clear();
}
