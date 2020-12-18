#include "StdAfx.h"
#include "zone_effector.h"
#include "Level.h"
#include "xrEngine/xr_object.h"
#include "xrEngine/CameraManager.h"
#include "Actor.h"
#include "ActorEffector.h"
#include "PostprocessAnimator.h"
#include "CustomOutfit.h"

CZoneEffector::CZoneEffector()
{
    m_pp_effector = NULL;
    m_pActor = NULL;
    m_factor = 0.1f;
}

CZoneEffector::~CZoneEffector() { Stop(); }
void CZoneEffector::Load(LPCSTR section)
{
    if (pSettings->line_exist(section, "pp_eff_name"))
        m_pp_fname = pSettings->r_string(section, "pp_eff_name");
    else if (pSettings->line_exist(section, "ppe_file"))
        m_pp_fname = pSettings->r_string(section, "ppe_file");
    else
        VERIFY2(pSettings->line_exist(section, "pp_eff_name"), section);

    r_min_perc = pSettings->r_float(section, "radius_min");
    r_max_perc = pSettings->r_float(section, "radius_max");
    VERIFY(r_min_perc <= r_max_perc);
}

#pragma warning(push)
#pragma warning(disable : 4826) // XXX: Do something with that cheap ID generation, remove warning
void CZoneEffector::Activate()
{
    m_pActor = smart_cast<CActor*>(Level().CurrentEntity());
    if (!m_pActor)
        return;
    m_pp_effector = xr_new<CPostprocessAnimatorLerp>();
    m_pp_effector->SetType(EEffectorPPType(u32(u64(this) & u32(-1))));
    m_pp_effector->SetCyclic(true);
    m_pp_effector->SetFactorFunc(GET_KOEFF_FUNC(this, &CZoneEffector::GetFactor));
    m_pp_effector->Load(*m_pp_fname);
    m_pActor->Cameras().AddPPEffector(m_pp_effector);
}

void CZoneEffector::Stop()
{
    if (!m_pp_effector)
        return;

    m_pActor->Cameras().RemovePPEffector(EEffectorPPType(u32(u64(this) & u32(-1))));
    m_pp_effector = NULL;
    m_pActor = NULL;
};
#pragma warning(pop)

void CZoneEffector::Update(float dist, float r, ALife::EHitType hit_type)
{
    float min_r = r * r_min_perc;
    float max_r = r * r_max_perc;

    bool camera_on_actor = (Level().CurrentEntity() && smart_cast<CActor*>(Level().CurrentEntity()));

    if (m_pp_effector)
    {
        if ((dist > max_r) || !camera_on_actor || (m_pActor && !m_pActor->g_Alive()))
            Stop();
    }
    else
    {
        if ((dist < max_r) && camera_on_actor)
            Activate();
    }

    float protection = 0.f;
    if (m_pActor)
    {
        CCustomOutfit* outfit = m_pActor->GetOutfit();
        if (outfit)
            protection = outfit->GetDefHitTypeProtection(hit_type);
    }
    if (m_pp_effector)
    {
        m_factor = ((max_r - dist) / (max_r - min_r)) - protection;
        clamp(m_factor, 0.01f, 1.0f);
    }
}

float CZoneEffector::GetFactor() { return m_factor; }
