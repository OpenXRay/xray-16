///////////////////////////////////////////////////////////////
// BlackGraviArtifact.cpp
// BlackGraviArtefact - гравитационный артефакт,
// такой же как и обычный, но при получении хита
///////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "BlackGraviArtifact.h"
#include "xrPhysics/PhysicsShell.h"
#include "entity_alive.h"
#include "ParticlesObject.h"
#include "PHMovementControl.h"
#include "xrMessages.h"
#include "PhysicsShellHolder.h"
#include "Explosive.h"
#include "xrPhysics/IPHWorld.h"
#include "CharacterPhysicsSupport.h"
// extern CPHWorld*	ph_world;
CBlackGraviArtefact::CBlackGraviArtefact(void)
{
    m_fImpulseThreshold = 10.f;
    m_fRadius = 10.f;
    m_fStrikeImpulse = 50.f;

    m_bStrike = false;
}

CBlackGraviArtefact::~CBlackGraviArtefact(void) { m_GameObjectList.clear(); }
void CBlackGraviArtefact::Load(LPCSTR section)
{
    inherited::Load(section);

    m_fImpulseThreshold = pSettings->r_float(section, "impulse_threshold");
    m_fRadius = pSettings->r_float(section, "radius");
    m_fStrikeImpulse = pSettings->r_float(section, "strike_impulse");
    m_sParticleName = pSettings->r_string(section, "particle");
}

BOOL CBlackGraviArtefact::net_Spawn(CSE_Abstract* DC)
{
    if (!inherited::net_Spawn(DC))
        return FALSE;

    CParticlesObject* pStaticPG;
    pStaticPG = CParticlesObject::Create("anomaly" DELIMITER "galantine", FALSE);
    Fmatrix pos;
    // pos.rotateY(1.57);
    // pos.mulA(pos);
    pos.scale(0.7f, 0.7f, 0.7f);
    pos.translate_over(XFORM().c);

    Fvector vel;
    vel.set(0, 0, 0);
    pStaticPG->UpdateParent(pos, vel);
    pStaticPG->Play(false);

    return TRUE;
}
struct SRP
{
    const CPhysicsShellHolder* obj;
    SRP(const CPhysicsShellHolder* O) { obj = O; }
    bool operator()(CPhysicsShellHolder* O) const { return obj == O; }
};
void CBlackGraviArtefact::net_Relcase(IGameObject* O)
{
    inherited::net_Relcase(O);
    // for vector
    auto I =
        std::remove_if(m_GameObjectList.begin(), m_GameObjectList.end(), SRP(smart_cast<CPhysicsShellHolder*>(O)));
    m_GameObjectList.erase(I, m_GameObjectList.end());
    // for list
    // m_GameObjectList.remove_if(SRP(smart_cast<CPhysicsShellHolder*>(O)));
}
void CBlackGraviArtefact::UpdateCLChild()
{
    VERIFY(!physics_world()->Processing());
    inherited::UpdateCLChild();

    if (getVisible() && m_pPhysicsShell)
    {
        if (m_bStrike)
        {
            Fvector P;
            P.set(Position());
            feel_touch_update(P, m_fRadius);

            GraviStrike();

            CParticlesObject* pStaticPG;
            pStaticPG = CParticlesObject::Create(*m_sParticleName, TRUE);
            Fmatrix pos;
            pos.set(XFORM());
            Fvector vel;
            // vel.sub(Position(),ps_Element(0).vPosition);
            // vel.div((Level().timeServer()-ps_Element(0).dwTime)/1000.f);
            vel.set(0, 0, 0);
            pStaticPG->UpdateParent(pos, vel);
            pStaticPG->Play(false);

            m_bStrike = false;
        }
    }
    else if (H_Parent())
        XFORM().set(H_Parent()->XFORM());
}

// void CBlackGraviArtefact::Hit(float P, Fvector &dir,
//						IGameObject* who, s16 element,
//						Fvector position_in_object_space,
//						float impulse,
//						ALife::EHitType hit_type)
void CBlackGraviArtefact::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    if (HDS.impulse > m_fImpulseThreshold)
    {
        m_bStrike = true;
        //чтоб выстрел не повлиял на траекторию полета артефакта
        HDS.impulse = 0;
    }

    //	inherited::Hit(P, dir, who, element, position_in_object_space, impulse, hit_type);
    inherited::Hit(&HDS);
}

void CBlackGraviArtefact::feel_touch_new(IGameObject* O)
{
    CPhysicsShellHolder* pGameObject = smart_cast<CPhysicsShellHolder*>(O);
    CArtefact* pArtefact = smart_cast<CArtefact*>(O);

    if (pGameObject && !pArtefact)
    {
        m_GameObjectList.push_back(pGameObject);
    }
}

void CBlackGraviArtefact::feel_touch_delete(IGameObject* O)
{
    CGameObject* pGameObject = smart_cast<CGameObject*>(O);
    CArtefact* pArtefact = smart_cast<CArtefact*>(O);

    if (pGameObject && !pArtefact)
    {
        m_GameObjectList.erase(std::find(m_GameObjectList.begin(), m_GameObjectList.end(), pGameObject));
    }
}

bool CBlackGraviArtefact::feel_touch_contact(IGameObject* O)
{
    CGameObject* pGameObject = smart_cast<CGameObject*>(O);

    if (pGameObject)
        return true;
    else
        return false;
}

void CBlackGraviArtefact::GraviStrike()
{
    xr_list<s16> elements_list;
    xr_list<Fvector> bone_position_list;

    Fvector object_pos;
    Fvector strike_dir;

    rq_storage.r_clear();

    for (auto it = m_GameObjectList.begin(); m_GameObjectList.end() != it; ++it)
    {
        CPhysicsShellHolder* pGameObject = *it;

        if (pGameObject->Visual())
            pGameObject->Center(object_pos);
        else
            object_pos.set(pGameObject->Position());

        strike_dir.sub(object_pos, Position());
        float distance = strike_dir.magnitude();

        float impulse = 100.f * m_fStrikeImpulse * (1.f - (distance / m_fRadius) * (distance / m_fRadius));

        if (impulse > .001f)
        {
            //?			BOOL		enabled = getEnabled();
            //?			setEnabled	(FALSE);
            impulse *= CExplosive::ExplosionEffect(rq_storage, NULL, pGameObject, Position(), m_fRadius);
            //?			setEnabled	(enabled);
        }

        float hit_power;
        CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(pGameObject);
        if (pGameObject->m_pPhysicsShell)
            hit_power = 0;
        else if (pEntityAlive && pEntityAlive->g_Alive() &&
            pEntityAlive->character_physics_support()->movement()->CharacterExist())
            hit_power = 0;
        else
            hit_power = impulse;

        if (impulse > .001f)
        {
            while (!elements_list.empty())
            {
                s16 element = elements_list.front();
                Fvector bone_pos = bone_position_list.front();

                NET_Packet P;
                SHit HS;
                HS.GenHeader(GE_HIT, pGameObject->ID()); //				u_EventGen		(P,GE_HIT, pGameObject->ID());
                HS.whoID = ID(); //				P.w_u16			(ID());
                HS.weaponID = ID(); //				P.w_u16			(ID());
                HS.dir = strike_dir; //				P.w_dir			(strike_dir);
                HS.power = hit_power; //				P.w_float		(hit_power);
                HS.boneID = element; //				P.w_s16			(element);
                HS.p_in_bone_space = bone_pos; //				P.w_vec3		(bone_pos);
                HS.impulse = impulse; //				P.w_float		(impulse);
                HS.hit_type = (ALife::eHitTypeWound); //				P.w_u16			(u16(ALife::eHitTypeWound));
                HS.Write_Packet(P);

                u_EventSend(P);
                elements_list.pop_front();
                bone_position_list.pop_front();
            }
        }
    }
}
