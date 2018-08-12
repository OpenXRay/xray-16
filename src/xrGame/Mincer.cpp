#include "StdAfx.h"
#include "alife_space.h"
#include "Hit.h"
#include "PHDestroyable.h"
#include "Mincer.h"
#include "xrMessages.h"
#include "Level.h"
#include "CustomZone.h"
#include "entity_alive.h"
#include "PHDestroyableNotificate.h"
#include "Actor.h"

CMincer::CMincer(void) { m_fActorBlowoutRadiusPercent = 0.5f; }
CMincer::~CMincer(void) {}
void CMincer::OnStateSwitch(EZoneState new_state)
{
    if (m_eZoneState != eZoneStateBlowout && new_state == eZoneStateBlowout)
    {
        for (auto it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it)
        {
            CPhysicsShellHolder* GO = smart_cast<CPhysicsShellHolder*>((*it).object);
            if (GO)
                Telekinesis().activate(GO, m_fThrowInImpulse, m_fTeleHeight, 100000);
        }
    }

    if (m_eZoneState == eZoneStateBlowout && new_state != eZoneStateBlowout)
    {
        Telekinesis().clear_deactivate();
    }
    inherited::OnStateSwitch(new_state);
}

void CMincer::Load(LPCSTR section)
{
    inherited::Load(section);

    m_telekinetics.set_destroing_particles(shared_str(pSettings->r_string(section, "tearing_particles")));
    m_telekinetics.set_throw_power(pSettings->r_float(section, "throw_out_impulse"));
    m_torn_particles = pSettings->r_string(section, "torn_particles");
    m_tearing_sound.create(pSettings->r_string(section, "body_tearing_sound"), st_Effect, sg_SourceType);
    m_fActorBlowoutRadiusPercent = pSettings->r_float(section, "actor_blowout_radius_percent");

    // pSettings->r_fvector3(section,whirlwind_center);
}

BOOL CMincer::net_Spawn(CSE_Abstract* DC)
{
    BOOL result = inherited::net_Spawn(DC);
    Fvector C;
    Center(C);
    C.y += m_fTeleHeight;
    m_telekinetics.SetCenter(C);
    m_telekinetics.SetOwnerObject(smart_cast<CGameObject*>(this));
    return result;
}
void CMincer::net_Destroy()
{
    inherited::net_Destroy();
    m_telekinetics.clear_impacts();
}
void CMincer::feel_touch_new(IGameObject* O)
{
    inherited::feel_touch_new(O);
    if (m_eZoneState == eZoneStateBlowout && (m_dwBlowoutExplosionTime > (u32)m_iStateTime))
    {
        CPhysicsShellHolder* GO = smart_cast<CPhysicsShellHolder*>(O);
        Telekinesis().activate(GO, m_fThrowInImpulse, m_fTeleHeight, 100000);
    }
}
bool CMincer::feel_touch_contact(IGameObject* O)
{
    return inherited::feel_touch_contact(O) && smart_cast<CPhysicsShellHolder*>(O);
}
void CMincer::AffectThrow(SZoneObjectInfo* O, CPhysicsShellHolder* GO, const Fvector& throw_in_dir, float dist)
{
    inherited::AffectThrow(O, GO, throw_in_dir, dist);
}

bool CMincer::BlowoutState()
{
    bool ret = inherited::BlowoutState();

    // xr_set<IGameObject*>::iterator it=m_inZone.begin(),e=m_inZone.end();
    // for(;e!=it;++it)
    //{
    //	CEntityAlive * EA = smart_cast<CEntityAlive *>(*it);
    //	if(!EA)continue;
    //	CPhysicsShellHolder * GO = smart_cast<CPhysicsShellHolder *>(*it);
    //	Telekinesis().activate(GO,m_fThrowInImpulse, m_fTeleHeight, 100000);

    //}

    if (m_dwBlowoutExplosionTime < (u32)m_iPreviousStateTime || m_dwBlowoutExplosionTime >= (u32)m_iStateTime)
        return ret;
    Telekinesis().deactivate();
    return ret;
}
void CMincer::ThrowInCenter(Fvector& C)
{
    C.set(m_telekinetics.Center());
    C.y = Position().y;
}

void CMincer::Center(Fvector& C) const { C.set(Position()); }
void CMincer::NotificateDestroy(CPHDestroyableNotificate* dn)
{
    Fvector dir;
    float power = 0.0f; // can change
    float impulse;
    // if(!m_telekinetics.has_impacts()) return;

    // IGameObject* obj=Level().Objects.net_Find(id);
    CPhysicsShellHolder* obj = dn->PPhysicsShellHolder();
    m_telekinetics.draw_out_impact(dir, impulse);
    CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(obj);
    if (PP && *m_torn_particles)
    {
        PP->StartParticles(m_torn_particles, Fvector().set(0, 1, 0), ID());
    }
    m_tearing_sound.play_at_pos(0, m_telekinetics.Center());

    Fvector position_in_bone_space, throw_in_dir;
    position_in_bone_space.set(0.0f, 0.0f, 0.0f);
    throw_in_dir.set(1.0f, 0.0f, 1.0f);
    CreateHit(obj->ID(), ID(), throw_in_dir, power, 0, position_in_bone_space, impulse, ALife::eHitTypeExplosion);
}

void CMincer::AffectPullAlife(CEntityAlive* EA, const Fvector& throw_in_dir, float dist)
{
    float power = Power(dist, Radius());
    // Fvector dir;
    // dir.random_dir(throw_in_dir,2.f*M_PI);
    if (!smart_cast<CActor*>(EA))
    {
        Fvector pos_in_bone_space;
        pos_in_bone_space.set(0, 0, 0);
        CreateHit(EA->ID(), ID(), throw_in_dir, power, 0, pos_in_bone_space, 0.0f, m_eHitTypeBlowout);
    }
    inherited::AffectPullAlife(EA, throw_in_dir, dist);
}

float CMincer::BlowoutRadiusPercent(CPhysicsShellHolder* GO)
{
    return (!smart_cast<CActor*>(GO) ? m_fBlowoutRadiusPercent : m_fActorBlowoutRadiusPercent);
}
