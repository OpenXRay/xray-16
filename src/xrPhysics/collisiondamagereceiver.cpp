#include "stdafx.h"

#include "icollisiondamagereceiver.h"
#include "IPhysicsShellHolder.h"

#include "ExtendedGeom.h"
#include "MathUtilsOde.h"

#include "xrEngine/GameMtlLib.h"

void DamageReceiverCollisionCallback(bool& do_colide, bool bo1, dContact& c, SGameMtl* material_1, SGameMtl* material_2)
{
    if (material_1->Flags.test(SGameMtl::flPassable) || material_2->Flags.test(SGameMtl::flPassable))
        return;
    dBodyID b1 = dGeomGetBody(c.geom.g1);
    dBodyID b2 = dGeomGetBody(c.geom.g2);
    dxGeomUserData* ud_self = bo1 ? retrieveGeomUserData(c.geom.g1) : retrieveGeomUserData(c.geom.g2);
    dxGeomUserData* ud_damager = bo1 ? retrieveGeomUserData(c.geom.g2) : retrieveGeomUserData(c.geom.g1);

    SGameMtl* material_self = bo1 ? material_1 : material_2;
    SGameMtl* material_damager = bo1 ? material_2 : material_1;
    VERIFY(ud_self);
    IPhysicsShellHolder* o_self = ud_self->ph_ref_object;
    IPhysicsShellHolder* o_damager = NULL;
    if (ud_damager)
        o_damager = ud_damager->ph_ref_object;
    u16 source_id = o_damager ? o_damager->ObjectID() : u16(-1);

    // CPHCollisionDamageReceiver	*dr	= static_cast<CPhysicsShellHolder*>( o_self )->PHCollisionDamageReceiver();
    ICollisionDamageReceiver* dr = (o_self)->ObjectPhCollisionDamageReceiver();
    VERIFY2(dr, "wrong callback");

    float damager_material_factor = material_damager->fBounceDamageFactor;

    if (ud_damager && ud_damager->ph_object && ud_damager->ph_object->CastType() == CPHObject::tpCharacter)
        o_damager->BonceDamagerCallback(damager_material_factor);

    // CCharacterPhysicsSupport* phs=static_cast<CPhysicsShellHolder*>(o_damager)->character_physics_support();
    // if(phs->IsSpecificDamager())damager_material_factor=phs->BonceDamageFactor();

    float dfs = (material_self->fBounceDamageFactor + damager_material_factor);
    if (fis_zero(dfs))
        return;
    Fvector dir;
    dir.set(*(Fvector*)c.geom.normal);
    Fvector pos;
    pos.sub(*(Fvector*)c.geom.pos,
        *(Fvector*)dGeomGetPosition(bo1 ? c.geom.g1 : c.geom.g2)); // it is not true pos in bone space

    dr->CollisionHit(
        source_id, ud_self->bone_id, E_NL(b1, b2, c.geom.normal) * damager_material_factor / dfs, dir, pos);
}

void BreakableObjectCollisionCallback(
    bool& /**do_colide/**/, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    dxGeomUserData* usr_data_1 = retrieveGeomUserData(c.geom.g1);
    dxGeomUserData* usr_data_2 = retrieveGeomUserData(c.geom.g2);
    VERIFY(usr_data_1);
    VERIFY(usr_data_2);
    // CBreakableObject* this_object	= 0;
    ICollisionDamageReceiver* damag_receiver = 0;

    dBodyID body = 0;
    float norm_sign = 0;

    if (bo1)
    {
        VERIFY(usr_data_1->ph_ref_object);
        damag_receiver = usr_data_1->ph_ref_object->ObjectPhCollisionDamageReceiver();
        body = dGeomGetBody(c.geom.g2);
        norm_sign = -1.f;
    }
    else
    {
        VERIFY(usr_data_2->ph_ref_object);
        damag_receiver = usr_data_2->ph_ref_object->ObjectPhCollisionDamageReceiver();
        body = dGeomGetBody(c.geom.g1);
        norm_sign = 1.f;
    }
    VERIFY(damag_receiver);

    /*

        CBreakableObject* this_object1	= 0;
        CBreakableObject* this_object2	= 0;
        VERIFY( usr_data_1 );
        VERIFY( usr_data_2 );
        this_object1 = smart_cast<CBreakableObject*>( usr_data_1->ph_ref_object );
        this_object2 = smart_cast<CBreakableObject*>( usr_data_2->ph_ref_object );
        if(
            usr_data_1&&
            usr_data_1->ph_ref_object&&
            this_object1
            )
        {
                body=dGeomGetBody(c.geom.g2);
                if(!body) return;
                this_object = this_object1;
                norm_sign=-1.f;
        }
        else if(
            usr_data_2&&
            usr_data_2->ph_ref_object&&
            this_object2
            )
        {
                body=dGeomGetBody(c.geom.g1);
                if(!body) return;
                this_object = this_object2;
                norm_sign=1.f;
        }
        else return;

        */
    // if(!this_object->m_pUnbrokenObject) return;

    float c_damage = E_NlS(body, c.geom.normal, norm_sign);
    Fvector dir =
        Fvector().set(-c.geom.normal[0] * norm_sign, -c.geom.normal[1] * norm_sign, -c.geom.normal[2] * norm_sign);
    Fvector pos = Fvector().set(c.geom.pos[0], c.geom.pos[1], c.geom.pos[2]);

    damag_receiver->CollisionHit(u16(-1), u16(-1), c_damage, dir, pos);

    // VERIFY( this_object->m_pUnbrokenObject );

    // if(this_object->m_damage_threshold<c_damage&&
    //	this_object->m_max_frame_damage<c_damage
    //	){
    //		this_object->b_resived_damage=true;
    //		this_object->m_max_frame_damage=c_damage;
    //		//this_object->m_contact_damage_pos.set(c.geom.pos[0],c.geom.pos[1],c.geom.pos[2]);
    //		this_object->m_contact_damage_pos.set( pos );
    //		//this_object->m_contact_damage_dir.set(-c.geom.normal[0]*norm_sign,-c.geom.normal[1]*norm_sign,-c.geom.normal[2]*norm_sign);
    //		this_object->m_contact_damage_dir.set( dir );
    //	}
}
