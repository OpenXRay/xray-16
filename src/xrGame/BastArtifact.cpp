///////////////////////////////////////////////////////////////
// BastArtifact.cpp
// BastArtefact - артефакт мочалка
///////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BastArtifact.h"
#include "xrPhysics/PhysicsShell.h"
#include "xrPhysics/extendedgeom.h"
#include "ParticlesObject.h"

CBastArtefact::CBastArtefact(void) : m_pHitedEntity(nullptr)
{
    m_fImpulseThreshold = 10.f;

    m_fRadius = 10.f;
    m_fStrikeImpulse = 15.f;

    m_bStrike = false;
    m_AttakingEntity = nullptr;

    m_fEnergy = 0.f;
    m_fEnergyMax = m_fStrikeImpulse * 100.f;
    m_fEnergyDecreasePerTime = 1.1f;
}

CBastArtefact::~CBastArtefact(void) {}
//вызывается при столкновении мочалки с чем-то
void CBastArtefact::ObjectContactCallback(
    bool& /**do_colide/**/, bool bo1, dContact& c, SGameMtl* /*material_1*/, SGameMtl* /*material_2*/)
{
    dxGeomUserData* l_pUD1 = NULL;
    dxGeomUserData* l_pUD2 = NULL;
    l_pUD1 = PHRetrieveGeomUserData(c.geom.g1);
    l_pUD2 = PHRetrieveGeomUserData(c.geom.g2);

    if (!l_pUD1 || !l_pUD2)
        return;

    //определить кто есть кто, из двух столкнувшихся предметов
    CBastArtefact* pBastArtefact = l_pUD1 ? smart_cast<CBastArtefact*>(l_pUD1->ph_ref_object) : NULL;
    if (!pBastArtefact)
        pBastArtefact = l_pUD2 ? smart_cast<CBastArtefact*>(l_pUD2->ph_ref_object) : NULL;
    if (!pBastArtefact)
        return;
    if (!pBastArtefact->IsAttacking())
        return;

    CEntityAlive* pEntityAlive = NULL;
    pEntityAlive = l_pUD1 ? smart_cast<CEntityAlive*>(l_pUD1->ph_ref_object) : NULL;
    if (!pEntityAlive)
        pEntityAlive = l_pUD2 ? smart_cast<CEntityAlive*>(l_pUD2->ph_ref_object) : NULL;

    pBastArtefact->BastCollision(pEntityAlive);
}

void CBastArtefact::BastCollision(CEntityAlive* pEntityAlive)
{
    //попали во что-то живое
    if (pEntityAlive && pEntityAlive->g_Alive())
    {
        m_AttakingEntity = NULL;
        m_pHitedEntity = pEntityAlive;

        if (m_AliveList.size() > 1)
        {
            m_bStrike = true;
        }
        else
        {
            m_bStrike = false;
        }

        m_bStrike = true;
        Fvector vel;
        vel.set(0, 0, 0);
        //	this->m_pPhysicsShell->set_LinearVel(vel);
        //	this->m_pPhysicsShell->set_AngularVel(vel);
    }
}

BOOL CBastArtefact::net_Spawn(CSE_Abstract* DC)
{
    BOOL result = inherited::net_Spawn(DC);
    if (!result)
        return FALSE;

    m_bStrike = false;
    m_AttakingEntity = NULL;
    m_pHitedEntity = NULL;
    m_AliveList.clear();

    return TRUE;
}

void CBastArtefact::net_Destroy()
{
    inherited::net_Destroy();

    m_bStrike = false;
    m_AttakingEntity = NULL;
    m_pHitedEntity = NULL;
    m_AliveList.clear();
}

void CBastArtefact::Load(LPCSTR section)
{
    inherited::Load(section);

    m_fImpulseThreshold = pSettings->r_float(section, "impulse_threshold");
    m_fRadius = pSettings->r_float(section, "radius");
    m_fStrikeImpulse = pSettings->r_float(section, "strike_impulse");

    m_fEnergyMax = pSettings->r_float(section, "energy_max");
    m_fEnergyDecreasePerTime = pSettings->r_float(section, "energy_decrease_speed");

    m_sParticleName = pSettings->r_string(section, "particle");
}

void CBastArtefact::shedule_Update(u32 dt)
{
    inherited::shedule_Update(dt);

    Fvector P;
    P.set(Position());
    feel_touch_update(P, m_fRadius);
}

void CBastArtefact::UpdateCLChild()
{
    // Log						("--- A - CBastArtefact",*cName());
    // Log						("--- A - CBastArtefact",renderable.xform);

    //современем энергия по немногу тоже уменьшается
    if (m_fEnergy > 0)
        m_fEnergy -= m_fEnergyDecreasePerTime * Device.fTimeDelta;

    if (getVisible() && m_pPhysicsShell)
    {
        if (m_bStrike)
        {
            //выбрать жертву, если она еще не выбрана
            if (!m_AliveList.empty() && m_AttakingEntity == NULL)
            {
                CEntityAlive* pEntityToHit = NULL;
                if (m_AliveList.size() > 1)
                {
                    do
                    {
                        int rnd = ::Random.randI(m_AliveList.size());
                        pEntityToHit = m_AliveList[rnd];
                    } while (pEntityToHit == m_pHitedEntity);
                }
                else
                {
                    pEntityToHit = m_AliveList.front();
                }

                m_AttakingEntity = pEntityToHit;
            }
        }

        if (m_AttakingEntity)
        {
            if (m_AttakingEntity->g_Alive() && m_fEnergy > m_fStrikeImpulse)
            {
                m_fEnergy -= m_fStrikeImpulse;

                //бросить артефакт на выбранную цель
                Fvector dir;
                m_AttakingEntity->Center(dir);
                dir.sub(this->Position());
                dir.y += ::Random.randF(-0.05f, 0.5f);

                m_pPhysicsShell->applyImpulse(dir, m_fStrikeImpulse * Device.fTimeDelta * m_pPhysicsShell->getMass());
            }
            else
            {
                m_AttakingEntity = NULL;
                m_bStrike = false;
            }
        }

        if (m_fEnergy > 0 && ::Random.randF(0.f, 1.0f) < (m_fEnergy / (m_fStrikeImpulse * 100.f)))
        {
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
        }
    }
    else if (H_Parent())
        XFORM().set(H_Parent()->XFORM());
}

// void CBastArtefact::Hit(float P, Fvector &dir,
//						IGameObject* who, s16 element,
//						Fvector position_in_object_space,
//						float impulse,
//						ALife::EHitType hit_type)
void CBastArtefact::Hit(SHit* pHDS)
{
    SHit HDS = *pHDS;
    if (HDS.impulse > m_fImpulseThreshold && !m_AliveList.empty())
    {
        m_bStrike = true;
        m_AttakingEntity = m_pHitedEntity = NULL;

        m_fEnergy += m_fStrikeImpulse * HDS.impulse;

        if (m_fEnergy > m_fEnergyMax)
            m_fEnergy = m_fEnergyMax;

        //чтоб выстрел не повлиял на траекторию полета артефакта
        HDS.impulse = 0;
    }

    //	inherited::Hit(P, dir, who, element, position_in_object_space, impulse, hit_type);
    inherited::Hit(&HDS);
}

//объект можно поднять только в спокойном состоянии
bool CBastArtefact::Useful() const
{
    if (m_fEnergy > 0)
        return false;
    else
        return true;
}

void CBastArtefact::feel_touch_new(IGameObject* O)
{
    CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(O);

    if (pEntityAlive && pEntityAlive->g_Alive())
    {
        m_AliveList.push_back(pEntityAlive);
    }
}

void CBastArtefact::feel_touch_delete(IGameObject* O)
{
    CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(O);

    if (pEntityAlive)
    {
        m_AliveList.erase(std::find(m_AliveList.begin(), m_AliveList.end(), pEntityAlive));
    }
}

bool CBastArtefact::feel_touch_contact(IGameObject* O)
{
    CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(O);

    if (pEntityAlive && pEntityAlive->g_Alive())
        return true;
    else
        return false;
}

void CBastArtefact::setup_physic_shell()
{
    inherited::setup_physic_shell();
    m_pPhysicsShell->set_PhysicsRefObject(this);
    m_pPhysicsShell->set_ObjectContactCallback(ObjectContactCallback);
    m_pPhysicsShell->set_ContactCallback(NULL);
}
