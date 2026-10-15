#pragma once
#include "PHSimpleCharacter.h"
#include "PHActorCharacterInline.h"
#include "ExtendedGeom.h"

class CPhysicShellHolder;
struct SPHCharacterRestrictor
{
    SPHCharacterRestrictor(ERestrictionType Ttype)
    {
        m_type = Ttype;
        m_character = nullptr;
        m_restrictor = nullptr;
        m_restrictor_transform = nullptr;
        m_restrictor_radius = 0.1f;
    }
    ~SPHCharacterRestrictor() { Destroy(); };

    CPHCharacter* m_character;
    ERestrictionType m_type;

    CPhysicsGeom* m_restrictor;
    CPhysicsGeom* m_restrictor_transform;
    float m_restrictor_radius;
    float m_restrictor_height = 0;

    void SetObjectContactCallback(ObjectContactCallbackFun* callback);
    void SetMaterial(u16 material);
    void Create(CPHCharacter* ch, Fvector sizes);
    void Destroy(void);
    void SetPhysicsRefObject(IPhysicsShellHolder* ref_object);
    void SetRadius(float r);
};

template <ERestrictionType Ttype>
struct TPHCharacterRestrictor : public SPHCharacterRestrictor
{
    TPHCharacterRestrictor() : SPHCharacterRestrictor(Ttype) {}
    void Create(CPHCharacter* ch, Fvector sizes)
    {
        // В оригинале здесь назначался коллбек на геометрию.
        // Предполагается, что CPhysicsGeom имеет соответствующий метод.
        if (m_restrictor)
            m_restrictor->set_contact_cb(RestrictorCallBack);
    }

    static void RestrictorCallBack(
        bool& do_colide, bool bo1,
        CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom,
        const Fvector& contact_normal, const Fvector& contact_pos,
        SGameMtl* material_1, SGameMtl* material_2
    )
    {
        do_colide = false;
        if (!my_geom || !oposite_geom)
            return;

        CPHObject* o1 = my_geom->ph_object;
        CPHObject* o2 = oposite_geom->ph_object;

        if (!(o1 && o2))
            return;
        if (o1->CastType() != CPHObject::tpCharacter || o2->CastType() != CPHObject::tpCharacter)
            return;

        CPHCharacter* ch1 = static_cast<CPHCharacter*>(o1);
        CPHCharacter* ch2 = static_cast<CPHCharacter*>(o2);

        if (bo1)
        {
            ch1->ChooseRestrictionType(rtStalker, ch2);
            do_colide = ch2->TouchRestrictor(Ttype);
        }
        else
        {
            ch2->ChooseRestrictionType(rtStalker, ch1);
            do_colide = ch1->TouchRestrictor(Ttype);
        }
    }
};

using RESRICTORS_V = xr_vector<SPHCharacterRestrictor*>;
using RESTRICTOR_I = RESRICTORS_V::iterator;

IC RESTRICTOR_I begin(RESRICTORS_V& v)
{
    return v.begin();
}

IC RESTRICTOR_I end(RESRICTORS_V& v)
{
    return v.end();
}

class CPHActorCharacter : public CPHSimpleCharacter
{
    typedef CPHSimpleCharacter inherited;

    RESRICTORS_V m_restrictors;
    float m_speed_goal;
    bool b_single_game;

public:
    typedef TPHCharacterRestrictor<rtStalker> stalker_restrictor;
    typedef TPHCharacterRestrictor<rtStalkerSmall> stalker_small_restrictor;
    typedef TPHCharacterRestrictor<rtMonsterMedium> medium_monster_restrictor;

public:
    virtual CPHActorCharacter* CastActorCharacter() { return this; }
    virtual void SetObjectContactCallback(ObjectContactCallbackFun* callback);
    virtual void SetMaterial(u16 material);
    virtual void Create(Fvector sizes);
    virtual void Destroy(void);
    virtual void SetPhysicsRefObject(IPhysicsShellHolder* ref_object);
    virtual void SetAcceleration(Fvector accel);
    virtual void Disable();
    virtual void Jump(const Fvector& jump_velocity);
    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2);
    virtual void SetRestrictorRadius(ERestrictionType rtype, float r);
    virtual void ChooseRestrictionType(ERestrictionType my_type, CPHCharacter* ch);
    CPhysicsGeom* NativeRestrictionGeometry(CPHCharacter* other);

    CPHActorCharacter(bool single_game);
    virtual ~CPHActorCharacter(void);

private:
    virtual void ValidateWalkOn();
    bool CanJump();
    virtual void update_last_material();
    virtual void PhTune(float step);

private:
    void ClearRestrictors();
    RESTRICTOR_I Restrictor(ERestrictionType rtype);
};
