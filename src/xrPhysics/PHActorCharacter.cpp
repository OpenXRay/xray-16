#include "StdAfx.h"
#include "PHActorCharacter.h"
#include "ExtendedGeom.h"
#include "PhysicsCommon.h"
#include "IPhysicsShellHolder.h"
#include "xrPhysicsCore/IPhysicsCore.h"

const float JUMP_UP_VELOCITY = 6.0f;
const float JUMP_INCREASE_VELOCITY_RATE = 1.2f;

CPHActorCharacter::CPHActorCharacter(bool single_game) : b_single_game(single_game)
{
    SetRestrictionType(rtActor);

    m_restrictors.resize(3);
    m_restrictors[0] = xr_new<stalker_restrictor>();
    m_restrictors[1] = xr_new<stalker_small_restrictor>();
    m_restrictors[2] = xr_new<medium_monster_restrictor>();
}

CPHActorCharacter::~CPHActorCharacter(void) { ClearRestrictors(); }

static u16 slide_material_index = GAMEMTL_NONE_IDX;

void CPHActorCharacter::Create(Fvector sizes)
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
        const auto mi = GMLib.GetMaterialIt("materials" DELIMITER "earth_slide");
        if (mi != GMLib.LastMaterial())
            slide_material_index = u16(mi - GMLib.FirstMaterial());
    }
}

void CPHActorCharacter::ValidateWalkOn()
{
    if (LastMaterialIDX() == slide_material_index)
        b_clamb_jump = false;
    else
        inherited::ValidateWalkOn();
}

void SPHCharacterRestrictor::Create(CPHCharacter* ch, Fvector sizes)
{
    VERIFY(ch);
    if (m_character)
        return;
    m_character = ch;

    m_restrictor_height = sizes.y;
    Fcylinder cylinder;
    cylinder.m_center.set(0, sizes.y / 2, 0);
    cylinder.m_direction.set(0, 1, 0);
    cylinder.m_radius = m_restrictor_radius;
    cylinder.m_height = sizes.y;
    m_restrictor = xr_new<CCylinderGeom>(cylinder);
    m_restrictor->init();
    m_restrictor->ph_object = ch;
    m_restrictor->b_static_colide = false;
    m_restrictor_transform = m_restrictor;

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
    R_ASSERT(r > 0 && _valid(r));
    m_restrictor_radius = r;
    if (m_character && m_restrictor)
    {
        auto* character = m_character;
        const auto height = m_restrictor_height;
        auto* holder = m_restrictor->ph_ref_object;
        const auto material = m_restrictor->material;
        Destroy();
        Create(character, Fvector().set(0, height, 0));
        SetPhysicsRefObject(holder);
        SetMaterial(material);
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
    xr_delete(m_restrictor);
    m_restrictor_transform = nullptr;
    m_character = nullptr;
}

CPhysicsGeom* CPHActorCharacter::NativeRestrictionGeometry(CPHCharacter* other)
{
    if (!other || !b_single_game || m_restrictors.empty()) return nullptr;
    const auto type = other->RestrictionType();
    return static_cast<size_t>(type) < m_restrictors.size() ? m_restrictors[type]->m_restrictor : nullptr;
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
    if (m_character && m_restrictor)
    {
        m_restrictor->ph_ref_object = ref_object;
    }
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

void SPHCharacterRestrictor::SetMaterial(u16 material)
{
    if (m_restrictor)
        m_restrictor->set_material(material);
}

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
        (m_ground_contact_normal.y > 0.5f || m_elevator_state.ClimbingState());
}

void CPHActorCharacter::Jump(const Fvector& accel)
{
    if (!b_exist)
        return;
    if (CanJump())
    {
        b_jump = true;

        Fvector vel;
        GetPhysicsCore()->GetCharacterVirtualVelocity(m_char_handle, vel);

        float amag = m_acceleration.magnitude();
        if (amag < 1.f)
            amag = 1.f;

        if (m_elevator_state.ClimbingState())
        {
            m_elevator_state.GetJumpDir(m_acceleration, m_jump_accel);
            m_jump_accel.mul(JUMP_UP_VELOCITY / 2.f);
        }
        else
        {
            m_jump_accel.set(
                vel.x * JUMP_INCREASE_VELOCITY_RATE + m_acceleration.x / amag * 0.2f,
                JUMP_UP_VELOCITY,
                vel.z * JUMP_INCREASE_VELOCITY_RATE + m_acceleration.z / amag * 0.2f
            );
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
    SFindPredicate(CPhysicsGeom* geom1, CPhysicsGeom* geom2, bool* b)
    {
        g1 = geom1;
        g2 = geom2;
        b1 = b;
    }
    bool* b1;
    CPhysicsGeom* g1;
    CPhysicsGeom* g2;

    bool operator()(SPHCharacterRestrictor* o)
    {
        *b1 = g1 == o->m_restrictor_transform;
        return *b1 || g2 == o->m_restrictor_transform;
    }
};

static void BigVelSeparate(bool& do_collide, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom)
{
    if (!do_collide)
        return;

    if (!my_geom || !oposite_geom || !my_geom->ph_object || !oposite_geom->ph_object)
        return;

    if (my_geom->ph_object->CastType() != CPHObject::tpCharacter ||
        oposite_geom->ph_object->CastType() != CPHObject::tpCharacter)
        return;

    // В Jolt Physics ручное создание контактов (суставов) для расталкивания объектов
    // с большими скоростями больше не требуется. Движок обрабатывает это корректно
    // через систему CCD и мягкие контакты (Soft Contacts).
    // Поэтому мы просто оставляем коллизию активной (do_collide = true).
}

void CPHActorCharacter::InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2)
{
    bool b1 = false;
    SFindPredicate fp(bo1 ? my_geom : oposite_geom, bo1 ? oposite_geom : my_geom, &b1);
    RESTRICTOR_I r = std::find_if(begin(m_restrictors), end(m_restrictors), fp);
    bool b_restrictor = (r != end(m_restrictors));

    SGameMtl* material_1 = GMLib.GetMaterialByIdx(material_idx_1);
    SGameMtl* material_2 = GMLib.GetMaterialByIdx(material_idx_2);

    if ((material_1 && material_1->Flags.test(SGameMtl::flActorObstacle)) ||
        (material_2 && material_2->Flags.test(SGameMtl::flActorObstacle)))
        do_collide = true;

    if (b_single_game)
    {
        if (b_restrictor)
        {
            b_side_contact = true;
            // c->surface.mu = 0.00f; // Управление трением передается в Jolt ContactListener
        }
        else
            inherited::InitContact(do_collide, bo1, depth, my_geom, oposite_geom, material_idx_1, material_idx_2);

        if (b_restrictor && do_collide &&
            !(b1 ? static_cast<CPHCharacter*>(oposite_geom->ph_object)->ActorMovable() :
                   static_cast<CPHCharacter*>(my_geom->ph_object)->ActorMovable()))
        {
            // ODE создавал здесь фейковый сустав. Мы просто форсируем включение объектов.
            Enable();
            do_collide = false;
            m_friction_factor *= 0.1f;
        }
    }
    else
    {
        if (my_geom && oposite_geom)
        {
            IPhysicsShellHolder* A1 = my_geom->ph_ref_object;
            IPhysicsShellHolder* A2 = oposite_geom->ph_ref_object;
            if (A1 && A2 && A1->IsActor() && A2->IsActor())
            {
                do_collide = do_collide && !b_restrictor && (A1->ObjectPPhysicsShell() == nullptr) == (A2->ObjectPPhysicsShell() == nullptr);
            }
        }

        if (do_collide)
            inherited::InitContact(do_collide, bo1, depth, my_geom, oposite_geom, material_idx_1, material_idx_2);

        BigVelSeparate(do_collide, my_geom, oposite_geom);
    }
}

void CPHActorCharacter::ChooseRestrictionType(ERestrictionType my_type, CPHCharacter* ch)
{
    if (my_type != rtStalker || (ch->RestrictionType() != rtStalker && ch->RestrictionType() != rtStalkerSmall))
        return;

    float checkR = m_restrictors[rtStalkerSmall]->m_restrictor_radius;

    switch (ch->RestrictionType())
    {
    case rtStalkerSmall:
        if (ch->ObjectRadius() > checkR)
        {
            ch->SetNewRestrictionType(rtStalker);
            Enable();
        }
        break;
    case rtStalker:
        if (ch->ObjectRadius() < checkR)
        {
            ch->SetRestrictionType(rtStalkerSmall);
            Enable();
        }
        break;
    default: NODEFAULT;
    }
}

void CPHActorCharacter::update_last_material()
{
    inherited::update_last_material();
}

float free_fly_up_force_limit = 4000.f;

void CPHActorCharacter::PhTune(float step)
{
    inherited::PhTune(step);
}
