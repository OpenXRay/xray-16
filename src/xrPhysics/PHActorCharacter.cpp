#include "stdafx.h"
#include "PHActorCharacter.h"
#include "ExtendedGeom.h"
#include "PhysicsCommon.h"

#include "IPhysicsShellHolder.h"

#include "xrEngine/GameMtlLib.h"

// const float JUMP_HIGHT=0.5;
const float JUMP_UP_VELOCITY = 6.0f; // 5.6f;
const float JUMP_INCREASE_VELOCITY_RATE = 1.2f;
//#ifdef DEBUG
// XRPHYSICS_API BOOL use_controllers_separation = TRUE;
//#endif
CPHActorCharacter::CPHActorCharacter(bool single_game) : b_single_game(single_game)
{
    SetRestrictionType(rtActor);

    // std::fill(m_restrictors_index,m_restrictors_index+CPHCharacter::rtNone,end(m_restrictors));
    // m_restrictors_index[CPHCharacter::rtStalker]		=begin(m_restrictors)+0;
    // m_restrictors_index[CPHCharacter::rtMonsterMedium]	=begin(m_restrictors)+1;

    {
        m_restrictors.resize(3);
        m_restrictors[0] = (new stalker_restrictor());
        m_restrictors[1] = new stalker_small_restrictor();
        m_restrictors[2] = (new medium_monster_restrictor());
    }
}

CPHActorCharacter::~CPHActorCharacter(void) { ClearRestrictors(); }
static u16 slide_material_index = GAMEMTL_NONE_IDX;
void CPHActorCharacter::Create(dVector3 sizes)
{
    if (b_exist)
        return;
    inherited::Create(sizes);
    if (!b_single_game)
    {
        ClearRestrictors();
    }
    RESTRICTOR_I i = begin(m_restrictors), e = end(m_restrictors);
    for (; e != i; ++i)
    {
        (*i)->Create(this, sizes);
    }

    if (m_phys_ref_object)
    {
        SetPhysicsRefObject(m_phys_ref_object);
    }
    if (slide_material_index == GAMEMTL_NONE_IDX)
    {
        GameMtlIt mi = GMLibrary().GetMaterialIt("materials" DELIMITER "earth_slide");
        if (mi != GMLibrary().LastMaterial())
            slide_material_index = u16(mi - GMLibrary().FirstMaterial());
        // slide_material_index = GMLibrary().GetMaterialIdx("earth_slide");
    }
}
void CPHActorCharacter::ValidateWalkOn()
{
    if (LastMaterialIDX() == slide_material_index)
        b_clamb_jump = false;
    else
        inherited::ValidateWalkOn();
}
void SPHCharacterRestrictor::Create(CPHCharacter* ch, dVector3 sizes)
{
    VERIFY(ch);
    if (m_character)
        return;
    m_character = ch;
    m_restrictor = dCreateCylinder(0, m_restrictor_radius, sizes[1]);
    dGeomSetPosition(m_restrictor, 0.f, sizes[1] / 2.f, 0.f);
    m_restrictor_transform = dCreateGeomTransform(0);
    dGeomTransformSetCleanup(m_restrictor_transform, 0);
    dGeomTransformSetInfo(m_restrictor_transform, 1);
    dGeomTransformSetGeom(m_restrictor_transform, m_restrictor);
    dGeomCreateUserData(m_restrictor);
    dGeomGetUserData(m_restrictor)->b_static_colide = false;

    dGeomSetBody(m_restrictor_transform, m_character->get_body());
    dSpaceAdd(m_character->dSpace(), m_restrictor_transform);
    dGeomUserDataSetPhObject(m_restrictor, (CPHObject*)m_character);
    switch (m_type)
    {
    case rtStalker: static_cast<CPHActorCharacter::stalker_restrictor*>(this)->Create(ch, sizes); break;
    case rtStalkerSmall: static_cast<CPHActorCharacter::stalker_small_restrictor*>(this)->Create(ch, sizes); break;
    case rtMonsterMedium: static_cast<CPHActorCharacter::medium_monster_restrictor*>(this)->Create(ch, sizes); break;
    default: NODEFAULT;
    }
}

RESTRICTOR_I CPHActorCharacter::Restrictor(ERestrictionType rtype)
{
    R_ASSERT2(rtype < rtActor, "not valide restrictor");
    return begin(m_restrictors) + rtype;
}
void CPHActorCharacter::SetRestrictorRadius(ERestrictionType rtype, float r)
{
    if (m_restrictors.size() > 0)
        (*Restrictor(rtype))->SetRadius(r);
}

void SPHCharacterRestrictor::SetRadius(float r)
{
    m_restrictor_radius = r;
    if (m_character)
    {
        float h;
        dGeomCylinderGetParams(m_restrictor, &r, &h);
        dGeomCylinderSetParams(m_restrictor, m_restrictor_radius, h);
    }
}
void CPHActorCharacter::Destroy()
{
    if (!b_exist)
        return;
    RESTRICTOR_I i = begin(m_restrictors), e = end(m_restrictors);
    for (; e != i; ++i)
    {
        (*i)->Destroy();
    }
    inherited::Destroy();
}
void CPHActorCharacter::ClearRestrictors()
{
    RESTRICTOR_I i = begin(m_restrictors), e = end(m_restrictors);
    for (; e != i; ++i)
    {
        (*i)->Destroy();
        xr_delete(*i);
    }
    m_restrictors.clear();
}
void SPHCharacterRestrictor::Destroy()
{
    if (m_restrictor)
    {
        dGeomDestroyUserData(m_restrictor);
        dGeomDestroy(m_restrictor);
        m_restrictor = NULL;
    }

    if (m_restrictor_transform)
    {
        dGeomDestroyUserData(m_restrictor_transform);
        m_restrictor_transform = NULL;
    }
    m_character = NULL;
}
void CPHActorCharacter::SetPhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    inherited::SetPhysicsRefObject(ref_object);
    RESTRICTOR_I i = begin(m_restrictors), e = end(m_restrictors);
    for (; e != i; ++i)
    {
        (*i)->SetPhysicsRefObject(ref_object);
    }
}
void SPHCharacterRestrictor::SetPhysicsRefObject(IPhysicsShellHolder* ref_object)
{
    if (m_character)
        dGeomUserDataSetPhysicsRefObject(m_restrictor, ref_object);
}
void CPHActorCharacter::SetMaterial(u16 material)
{
    inherited::SetMaterial(material);
    if (!b_exist)
        return;
    RESTRICTOR_I i = begin(m_restrictors), e = end(m_restrictors);
    for (; e != i; ++i)
    {
        (*i)->SetMaterial(material);
    }
}
void SPHCharacterRestrictor::SetMaterial(u16 material) { dGeomGetUserData(m_restrictor)->material = material; }
void CPHActorCharacter::SetAcceleration(Fvector accel)
{
    Fvector cur_a, input_a;
    float cur_mug, input_mug;
    cur_a.set(m_acceleration);
    cur_mug = m_acceleration.magnitude();
    if (!fis_zero(cur_mug))
        cur_a.mul(1.f / cur_mug);
    input_a.set(accel);
    input_mug = accel.magnitude();
    if (!fis_zero(input_mug))
        input_a.mul(1.f / input_mug);
    if (!cur_a.similar(input_a, 0.05f) || !fis_zero(input_mug - cur_mug, 0.5f))
        inherited::SetAcceleration(accel);
}
bool CPHActorCharacter::CanJump()
{
    return !b_lose_control && LastMaterialIDX() != slide_material_index &&
        (m_ground_contact_normal[1] > 0.5f || m_elevator_state.ClimbingState());
}
void CPHActorCharacter::Jump(const Fvector& accel)
{
    if (!b_exist)
        return;
    if (CanJump())
    {
        b_jump = true;
        const dReal* vel = dBodyGetLinearVel(m_body);
        dReal amag = m_acceleration.magnitude();
        if (amag < 1.f)
            amag = 1.f;
        if (m_elevator_state.ClimbingState())
        {
            m_elevator_state.GetJumpDir(m_acceleration, m_jump_accel);
            m_jump_accel.mul(JUMP_UP_VELOCITY / 2.f);
            // if(accel.square_magnitude()>EPS_L)m_jump_accel.mul(4.f);
        }
        else
        {
            m_jump_accel.set(vel[0] * JUMP_INCREASE_VELOCITY_RATE + m_acceleration.x / amag * 0.2f, jump_up_velocity,
                vel[2] * JUMP_INCREASE_VELOCITY_RATE + m_acceleration.z / amag * 0.2f);
        }
        Enable();
    }
}
void CPHActorCharacter::SetObjectContactCallback(ObjectContactCallbackFun* callback)
{
    inherited::SetObjectContactCallback(callback);
}

void CPHActorCharacter::Disable() { inherited::Disable(); }
struct SFindPredicate
{
    SFindPredicate(const dContact* ac, bool* b)
    {
        c = ac;
        b1 = b;
    }
    bool* b1;
    const dContact* c;
    bool operator()(SPHCharacterRestrictor* o)
    {
        *b1 = c->geom.g1 == o->m_restrictor_transform;
        return *b1 || c->geom.g2 == o->m_restrictor_transform;
    }
};

static void BigVelSeparate(dContact* c, bool& do_collide)
{
    VERIFY(c);
#ifdef DEBUG
// if( !use_controllers_separation )
// return;
#endif
    if (!do_collide)
        return;
    dxGeomUserData* dat1 = retrieveGeomUserData(c->geom.g1);
    dxGeomUserData* dat2 = retrieveGeomUserData(c->geom.g2);

    if (!dat1 || !dat2 || !dat1->ph_object || !dat2->ph_object ||
        dat1->ph_object->CastType() != CPHObject::tpCharacter || dat2->ph_object->CastType() != CPHObject::tpCharacter)
        return;

    // float spr	= Spring( c->surface.soft_cfm,c->surface.soft_erp);
    // float dmp	= Damping( c->surface.soft_cfm,c->surface.soft_erp);
    // spr *=0.001f;
    // dmp *=10.f;
    // float cfm	= Cfm( spr, dmp );
    // float e		= Erp( spr, dmp );
    c->surface.soft_cfm *= 100.f;
    c->surface.soft_erp *= 0.1f;
    // MulSprDmp(c->surface.soft_cfm,c->surface.soft_erp ,0.1f,10);

    CPHCharacter* ch1 = static_cast<CPHCharacter*>(dat1->ph_object);
    CPHCharacter* ch2 = static_cast<CPHCharacter*>(dat2->ph_object);
    Fvector v1, v2;
    ch1->GetVelocity(v1);
    ch2->GetVelocity(v2);
    if (v1.square_magnitude() < 4.f && v2.square_magnitude() < 4.f)
        return;
    c->surface.mu = 1.00f;

    dJointID contact_joint1 = dJointCreateContactSpecial(0, ContactGroup, c);
    dJointID contact_joint2 = dJointCreateContactSpecial(0, ContactGroup, c);

    ch1->Enable();
    ch2->Enable();

    dat1->ph_object->Island().DActiveIsland()->ConnectJoint(contact_joint1);
    dat2->ph_object->Island().DActiveIsland()->ConnectJoint(contact_joint2);

    dJointAttach(contact_joint1, dGeomGetBody(c->geom.g1), 0);

    dJointAttach(contact_joint2, 0, dGeomGetBody(c->geom.g2));

    do_collide = false;
}

void CPHActorCharacter::InitContact(dContact* c, bool& do_collide, u16 material_idx_1, u16 material_idx_2)
{
    bool b1;
    SFindPredicate fp(c, &b1);
    RESTRICTOR_I r = std::find_if(begin(m_restrictors), end(m_restrictors), fp);
    bool b_restrictor = (r != end(m_restrictors));
    SGameMtl* material_1 = GMLibrary().GetMaterialByIdx(material_idx_1);
    SGameMtl* material_2 = GMLibrary().GetMaterialByIdx(material_idx_2);
    if ((material_1 && material_1->Flags.test(SGameMtl::flActorObstacle)) ||
        (material_2 && material_2->Flags.test(SGameMtl::flActorObstacle)))
        do_collide = true;
    if (b_single_game)
    {
        if (b_restrictor)
        {
            b_side_contact = true;
            // MulSprDmp(c->surface.soft_cfm,c->surface.soft_erp,def_spring_rate,def_dumping_rate);
            c->surface.mu = 0.00f;
        }
        else
            inherited::InitContact(c, do_collide, material_idx_1, material_idx_2);
        if (b_restrictor && do_collide &&
            !(b1 ? static_cast<CPHCharacter*>(retrieveGeomUserData(c->geom.g2)->ph_object)->ActorMovable() :
                   static_cast<CPHCharacter*>(retrieveGeomUserData(c->geom.g1)->ph_object)->ActorMovable()))
        {
            dJointID contact_joint = dJointCreateContactSpecial(0, ContactGroup, c);
            Enable();
            CPHObject::Island().DActiveIsland()->ConnectJoint(contact_joint);
            if (b1)
                dJointAttach(contact_joint, dGeomGetBody(c->geom.g1), 0);
            else
                dJointAttach(contact_joint, 0, dGeomGetBody(c->geom.g2));
            do_collide = false;
            m_friction_factor *= 0.1f;
        }
        // BigVelSeparate( c, do_collide );
    }
    else
    {
        dxGeomUserData* D1 = retrieveGeomUserData(c->geom.g1);
        dxGeomUserData* D2 = retrieveGeomUserData(c->geom.g2);
        if (D1 && D2)
        {
            IPhysicsShellHolder* A1 = (D1->ph_ref_object);
            IPhysicsShellHolder* A2 = (D2->ph_ref_object);
            if (A1 && A2 && A1->IsActor() && A2->IsActor())
            {
                do_collide =
                    do_collide && !b_restrictor && (A1->ObjectPPhysicsShell() == 0) == (A2->ObjectPPhysicsShell() == 0);
                c->surface.mu = 1.f;
            }
        }

        if (do_collide)
            inherited::InitContact(c, do_collide, material_idx_1, material_idx_2);
        BigVelSeparate(c, do_collide);
    }
}

void CPHActorCharacter::ChooseRestrictionType(ERestrictionType my_type, float my_depth, CPHCharacter* ch)
{
    if (my_type != rtStalker || (ch->RestrictionType() != rtStalker && ch->RestrictionType() != rtStalkerSmall))
        return;
    float checkR = m_restrictors[rtStalkerSmall]
                       ->m_restrictor_radius; // 1.5f;//+m_restrictors[rtStalker]->m_restrictor_radius)/2.f;

    switch (ch->RestrictionType())
    {
    case rtStalkerSmall:
        if (ch->ObjectRadius() > checkR)
        {
            // if(my_depth>0.05f)
            ch->SetNewRestrictionType(rtStalker);
            Enable();
// else ch->SetRestrictionType(rtStalker);
#ifdef DEBUG
            if (debug_output().ph_dbg_draw_mask1().test(ph_m1_DbgActorRestriction))
                Msg("restriction ready to change small -> large");
#endif
        }
        break;
    case rtStalker:
        if (ch->ObjectRadius() < checkR)
        {
#ifdef DEBUG
            if (debug_output().ph_dbg_draw_mask1().test(ph_m1_DbgActorRestriction))
                Msg("restriction  change large ->  small");
#endif
            ch->SetRestrictionType(rtStalkerSmall);
            Enable();
        }
        break;
    default: NODEFAULT;
    }
}

void CPHActorCharacter::update_last_material()
{
    if (ignore_material(*p_lastMaterialIDX))
        inherited::update_last_material();
}

float free_fly_up_force_limit = 4000.f;
void CPHActorCharacter::PhTune(dReal step)
{
    inherited::PhTune(step);
    if (b_lose_control && !b_external_impulse) //
    {
        const float* force = dBodyGetForce(m_body);
        float fy = force[1];
        if (fy > free_fly_up_force_limit)
            fy = free_fly_up_force_limit;
        dBodySetForce(m_body, force[0], fy, force[2]);
    }
}
