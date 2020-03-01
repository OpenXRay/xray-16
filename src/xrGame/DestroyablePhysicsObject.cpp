#include "pch_script.h"
#include "PHCollisionDamageReceiver.h"
#include "PhysicObject.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "hit_immunity.h"
#include "damage_manager.h"
#include "DestroyablePhysicsObject.h"
#include "Include/xrRender/KinematicsAnimated.h"
#include "Include/xrRender/Kinematics.h"
#include "xrServer_Objects_ALife.h"
#include "game_object_space.h"
#include "xrScriptEngine/script_callback_ex.h"
#include "script_game_object.h"
#include "xrPhysics/PhysicsShell.h"
#ifdef DEBUG
#include "xrPhysics/IPHWorld.h"

// extern CPHWorld			*ph_world;
#endif
CDestroyablePhysicsObject::CDestroyablePhysicsObject() { m_fHealth = 1.f; }
CDestroyablePhysicsObject::~CDestroyablePhysicsObject() {}
void CDestroyablePhysicsObject::OnChangeVisual()
{
    if (m_pPhysicsShell)
    {
        m_pPhysicsShell->Deactivate();
        xr_delete(m_pPhysicsShell);
        VERIFY(0 == Visual());
    }
    inherited::OnChangeVisual();
}
CPhysicsShellHolder* CDestroyablePhysicsObject::PPhysicsShellHolder() { return cast_physics_shell_holder(); }
void CDestroyablePhysicsObject::net_Destroy()
{
    inherited::net_Destroy();
    CPHDestroyable::RespawnInit();
    CPHCollisionDamageReceiver::Clear();
}

BOOL CDestroyablePhysicsObject::net_Spawn(CSE_Abstract* DC)
{
    BOOL res = inherited::net_Spawn(DC);
    IKinematics* K = smart_cast<IKinematics*>(Visual());
    CInifile* ini = K->LL_UserData();
    // R_ASSERT2(ini->section_exist("destroyed"),"destroyable_object must have -destroyed- section in model user data");
    CPHDestroyable::Init();
    if (ini && ini->section_exist("destroyed"))
        CPHDestroyable::Load(ini, "destroyed");

    CDamageManager::reload("damage_section", ini);
    if (ini)
    {
        if (ini->section_exist("immunities"))
            CHitImmunity::LoadImmunities("immunities", ini);
        CPHCollisionDamageReceiver::Init();
        if (ini->section_exist("sound"))
            m_destroy_sound.create(ini->r_string("sound", "break_sound"), st_Effect, sg_SourceType);
        if (ini->section_exist("particles"))
            m_destroy_particles = ini->r_string("particles", "destroy_particles");
    }
    CParticlesPlayer::LoadParticles(K);
    RunStartupAnim(DC);
    return res;
}

// void CDestroyablePhysicsObject::Hit							(float P,Fvector &dir,IGameObject *who,s16
// element,Fvector
// p_in_object_space, float impulse,  ALife::EHitType hit_type)
void CDestroyablePhysicsObject::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    callback(GameObject::eHit)(
        lua_game_object(), HDS.power, HDS.dir, smart_cast<const CGameObject*>(HDS.who)->lua_game_object(), HDS.bone());
    HDS.power = CHitImmunity::AffectHit(HDS.power, HDS.hit_type);
    float hit_scale = 1.f, wound_scale = 1.f;
    CDamageManager::HitScale(HDS.bone(), hit_scale, wound_scale);
    HDS.power *= hit_scale;
    //	inherited::Hit(P,dir,who,element,p_in_object_space,impulse,hit_type);
    inherited::Hit(&HDS);
    m_fHealth -= HDS.power;
    if (m_fHealth <= 0.f)
    {
        //		CPHDestroyable::SetFatalHit(SHit(P,dir,who,element,p_in_object_space,impulse,hit_type));
        CPHDestroyable::SetFatalHit(HDS);
        if (CPHDestroyable::CanDestroy())
            Destroy();
    }
}
void CDestroyablePhysicsObject::Destroy()
{
    VERIFY(!physics_world()->Processing());
    const CGameObject* who_object = smart_cast<const CGameObject*>(FatalHit().initiator());
    callback(GameObject::eDeath)(lua_game_object(), who_object ? who_object->lua_game_object() : 0);
    CPHDestroyable::Destroy(ID(), "physic_destroyable_object");
    if (m_destroy_sound._handle())
    {
        m_destroy_sound.play_at_pos(this, Position());
    }
    if (*m_destroy_particles)
    {
        // Fvector dir;dir.set(0,1,0);
        Fmatrix m;
        m.identity();
        /////////////////////////////////////////////////
        m.j.set(0, 1.f, 0);
        ///////////////////////////////////////////////

        Fvector hdir;
        hdir.set(CPHDestroyable::FatalHit().direction());

        if (fsimilar(_abs(m.j.dotproduct(hdir)), 1.f, EPS_L))
        {
            do
            {
                hdir.random_dir();
            } while (fsimilar(_abs(m.j.dotproduct(hdir)), 1.f, EPS_L));
        }
        m.i.crossproduct(m.j, hdir);
        m.i.normalize();
        m.k.crossproduct(m.i, m.j);
        StartParticles(m_destroy_particles, m, ID());
    }
    SheduleRegister();
}
void CDestroyablePhysicsObject::InitServerObject(CSE_Abstract* D)
{
    CSE_PHSkeleton* ps = smart_cast<CSE_PHSkeleton*>(D);
    R_ASSERT(ps);
    if (ps->_flags.test(CSE_PHSkeleton::flSpawnCopy))
        inherited::InitServerObject(D);
    else
        CPHDestroyable::InitServerObject(D);

    CSE_ALifeObjectPhysic* PO = smart_cast<CSE_ALifeObjectPhysic*>(D);
    if (PO)
        PO->type = epotSkeleton;
}
void CDestroyablePhysicsObject::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);
    CPHDestroyable::SheduleUpdate(dt);
}

bool CDestroyablePhysicsObject::CanRemoveObject()
{
    return !CParticlesPlayer::IsPlaying() && !m_destroy_sound._feedback(); //&& sound!
}
IFactoryObject* CDestroyablePhysicsObject::_construct()
{
    CDamageManager::_construct();
    return inherited::_construct();
}
