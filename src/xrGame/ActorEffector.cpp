#include "stdafx.h"

#include "ActorEffector.h"
#include "PostprocessAnimator.h"
#include "xrEngine/EffectorPP.h"
#include "xrEngine/ObjectAnimator.h"
#include "Common/object_broker.h"
#include "actor.h"

void AddEffector(CActor* A, int type, const shared_str& sect_name)
{
    if (pSettings->line_exist(sect_name, "pp_eff_name"))
    {
        CPostprocessAnimator* pp_anm = new CPostprocessAnimator();

        bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");

        pp_anm->bOverlap = !!pSettings->r_bool(sect_name, "pp_eff_overlap");

        pp_anm->SetType((EEffectorPPType)type);
        pp_anm->SetCyclic(bCyclic);

        LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
        pp_anm->Load(fn);
        A->Cameras().AddPPEffector(pp_anm);
    }
    if (pSettings->line_exist(sect_name, "cam_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
        CAnimatorCamEffector* cam_anm = new CAnimatorCamEffector();
        cam_anm->SetType((ECamEffectorType)type);
        cam_anm->SetCyclic(bCyclic);

        if (pSettings->line_exist(sect_name, "cam_eff_hud_affect"))
        {
            bool b_hud_affect = !!pSettings->r_bool(sect_name, "cam_eff_hud_affect");
            cam_anm->SetHudAffect(b_hud_affect);
        }

        LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
        cam_anm->Start(fn);
        A->Cameras().AddCamEffector(cam_anm);
    }
}

void AddEffector(CActor* A, int type, const shared_str& sect_name, CEffectorController* ec)
{
    if (pSettings->line_exist(sect_name, "pp_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
        CPostprocessAnimatorControlled* pp_anm = new CPostprocessAnimatorControlled(ec);
        pp_anm->SetType((EEffectorPPType)type);
        pp_anm->SetCyclic(bCyclic);
        pp_anm->bOverlap = !!pSettings->r_bool(sect_name, "pp_eff_overlap");
        LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
        pp_anm->Load(fn);
        A->Cameras().AddPPEffector(pp_anm);
    }
    if (pSettings->line_exist(sect_name, "cam_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
        CCameraEffectorControlled* cam_anm = new CCameraEffectorControlled(ec);
        cam_anm->SetType((ECamEffectorType)type);
        cam_anm->SetCyclic(bCyclic);

        if (pSettings->line_exist(sect_name, "cam_eff_hud_affect"))
        {
            bool b_hud_affect = !!pSettings->r_bool(sect_name, "cam_eff_hud_affect");
            cam_anm->SetHudAffect(b_hud_affect);
        }

        LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
        cam_anm->Start(fn);
        A->Cameras().AddCamEffector(cam_anm);
    }
}

void AddEffector(CActor* A, int type, const shared_str& sect_name, GET_KOEFF_FUNC k_func)
{
    if (pSettings->line_exist(sect_name, "pp_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
        CPostprocessAnimatorLerp* pp_anm = new CPostprocessAnimatorLerp();
        pp_anm->SetType((EEffectorPPType)type);
        pp_anm->SetCyclic(bCyclic);
        pp_anm->bOverlap = !!pSettings->r_bool(sect_name, "pp_eff_overlap");
        LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
        pp_anm->SetFactorFunc(k_func);
        pp_anm->Load(fn);
        A->Cameras().AddPPEffector(pp_anm);
    }
    if (pSettings->line_exist(sect_name, "cam_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
        CAnimatorCamLerpEffector* cam_anm = new CAnimatorCamLerpEffector();
        cam_anm->SetFactorFunc(k_func);
        cam_anm->SetType((ECamEffectorType)type);
        cam_anm->SetCyclic(bCyclic);

        if (pSettings->line_exist(sect_name, "cam_eff_hud_affect"))
        {
            bool b_hud_affect = !!pSettings->r_bool(sect_name, "cam_eff_hud_affect");
            cam_anm->SetHudAffect(b_hud_affect);
        }

        LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
        cam_anm->Start(fn);
        A->Cameras().AddCamEffector(cam_anm);
    }
};

void AddEffector(CActor* A, int type, const shared_str& sect_name, float factor)
{
    clamp(factor, 0.001f, 1.5f);
    if (pSettings->line_exist(sect_name, "pp_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "pp_eff_cyclic");
        CPostprocessAnimatorLerpConst* pp_anm = new CPostprocessAnimatorLerpConst();
        pp_anm->SetType((EEffectorPPType)type);
        pp_anm->SetCyclic(bCyclic);
        pp_anm->SetPower(factor);
        pp_anm->bOverlap = !!pSettings->r_bool(sect_name, "pp_eff_overlap");
        LPCSTR fn = pSettings->r_string(sect_name, "pp_eff_name");
        pp_anm->Load(fn);
        A->Cameras().AddPPEffector(pp_anm);
    }
    if (pSettings->line_exist(sect_name, "cam_eff_name"))
    {
        bool bCyclic = !!pSettings->r_bool(sect_name, "cam_eff_cyclic");
        CAnimatorCamLerpEffectorConst* cam_anm = new CAnimatorCamLerpEffectorConst();
        cam_anm->SetFactor(factor);
        cam_anm->SetType((ECamEffectorType)type);
        cam_anm->SetCyclic(bCyclic);

        if (pSettings->line_exist(sect_name, "cam_eff_hud_affect"))
        {
            bool b_hud_affect = !!pSettings->r_bool(sect_name, "cam_eff_hud_affect");
            cam_anm->SetHudAffect(b_hud_affect);
        }

        LPCSTR fn = pSettings->r_string(sect_name, "cam_eff_name");
        cam_anm->Start(fn);
        A->Cameras().AddCamEffector(cam_anm);
    }
};

void RemoveEffector(CActor* A, int type)
{
    A->Cameras().RemoveCamEffector((ECamEffectorType)type);
    A->Cameras().RemovePPEffector((EEffectorPPType)type);
}

CEffectorController::~CEffectorController() { R_ASSERT(!m_ce && !m_pe); }
CAnimatorCamEffector::CAnimatorCamEffector()
{
    m_bCyclic = true;
    m_objectAnimator = new CObjectAnimator();
    m_bAbsolutePositioning = false;
    m_fov = -1.0f;
}

CAnimatorCamEffector::~CAnimatorCamEffector() { delete_data(m_objectAnimator); }
void CAnimatorCamEffector::Start(LPCSTR fn)
{
    m_objectAnimator->Load(fn);
    m_objectAnimator->Play(Cyclic());
    fLifeTime = m_objectAnimator->GetLength();
}

BOOL CAnimatorCamEffector::Valid()
{
    if (Cyclic())
        return TRUE;
    return inherited::Valid();
}

BOOL CAnimatorCamEffector::ProcessCam(SCamEffectorInfo& info)
{
    if (!inherited::ProcessCam(info))
        return FALSE;

    const Fmatrix& m = m_objectAnimator->XFORM();
    m_objectAnimator->Update(Device.fTimeDelta);

    if (!m_bAbsolutePositioning)
    {
        Fmatrix Mdef;
        Mdef.identity();
        Mdef.j = info.n;
        Mdef.k = info.d;
        Mdef.i.crossproduct(info.n, info.d);
        Mdef.c = info.p;
        //		Msg("fr[%d] %2.3f,%2.3f,%2.3f", Device.dwFrame,m.c.x,m.c.y,m.c.z);
        Fmatrix mr;
        mr.mul(Mdef, m);
        info.d = mr.k;
        info.n = mr.j;
        info.p = mr.c;
    }
    else
    {
        info.d = m.k;
        info.n = m.j;
        info.p = m.c;
    };
    if (m_fov > 0.0f)
        info.fFov = m_fov;

    return TRUE;
}

BOOL CAnimatorCamLerpEffector::ProcessCam(SCamEffectorInfo& info)
{
    if (!CEffectorCam::ProcessCam(info))
        return FALSE;

    const Fmatrix& m = m_objectAnimator->XFORM();
    m_objectAnimator->Update(Device.fTimeDelta);

    Fmatrix Mdef;
    Mdef.identity();
    Mdef.j = info.n;
    Mdef.k = info.d;
    Mdef.i.crossproduct(info.n, info.d);
    Mdef.c = info.p;

    Fmatrix mr;
    mr.mul(Mdef, m);

    Fquaternion q_src, q_dst, q_res;
    q_src.set(Mdef);
    q_dst.set(mr);

    float t = m_func();
    clamp(t, 0.0f, 1.0f);

    VERIFY(t >= 0.f && t <= 1.f);
    q_res.slerp(q_src, q_dst, t);

    Fmatrix res;
    res.rotation(q_res);
    res.c.lerp(info.p, mr.c, t);

    info.d = res.k;
    info.n = res.j;
    info.p = res.c;

    if (m_fov > 0.0f)
        info.fFov = m_fov;

    return TRUE;
}

CAnimatorCamLerpEffectorConst::CAnimatorCamLerpEffectorConst() : m_factor(0.0f)
{
    SetFactorFunc(GET_KOEFF_FUNC(this, &CAnimatorCamLerpEffectorConst::GetFactor));
}

CCameraEffectorControlled::CCameraEffectorControlled(CEffectorController* c) : m_controller(c)
{
    m_controller->SetCam(this);
    SetFactorFunc(GET_KOEFF_FUNC(m_controller, &CEffectorController::GetFactor));
}

CCameraEffectorControlled::~CCameraEffectorControlled() { m_controller->SetCam(NULL); }
BOOL CCameraEffectorControlled::Valid() { return m_controller->Valid(); }
#define SND_MIN_VOLUME_FACTOR (0.1f)

SndShockEffector::SndShockEffector() : m_end_time(0), m_life_time(0)
{
    m_snd_length = 0.0f;
    m_cur_length = 0.0f;
    m_stored_volume = -1.0f;
    m_actor = nullptr;
}

SndShockEffector::~SndShockEffector()
{
    psSoundVFactor = m_stored_volume;
    if (m_actor && (m_ce || m_pe))
        RemoveEffector(m_actor, effHit);

    R_ASSERT(!m_ce && !m_pe);
}

BOOL SndShockEffector::Valid() { return (m_cur_length <= m_snd_length); }
BOOL SndShockEffector::InWork() { return inherited::Valid(); }
float SndShockEffector::GetFactor()
{
    float f = (m_end_time - Device.fTimeGlobal) / m_life_time;

    float ff = f * m_life_time / 8.0f;
    return clampr(ff, 0.0f, 1.0f);
}

void SndShockEffector::Start(CActor* A, float snd_length, float power)
{
    clamp(power, 0.1f, 1.5f);
    m_actor = A;
    m_snd_length = snd_length;

    if (m_stored_volume < 0.0f)
        m_stored_volume = psSoundVFactor;

    m_cur_length = 0;
    psSoundVFactor = m_stored_volume * SND_MIN_VOLUME_FACTOR;

    static float xxx = 6.0f / 1.50f; // 6sec on max power(1.5)

    m_life_time = power * xxx;
    m_end_time = Device.fTimeGlobal + m_life_time;

    AddEffector(A, effHit, "snd_shock_effector", this);
}

void SndShockEffector::Update()
{
    m_cur_length += Device.dwTimeDelta;
    float x = float(m_cur_length) / m_snd_length;
    float y = 2.f * x - 1;
    if (y > 0.f)
    {
        psSoundVFactor =
            y * (m_stored_volume - m_stored_volume * SND_MIN_VOLUME_FACTOR) + m_stored_volume * SND_MIN_VOLUME_FACTOR;
    }
}

//////////////////////////////////////////////////////////////////////////

#define DELTA_ANGLE_X 0.5f * PI / 180
#define DELTA_ANGLE_Y 0.5f * PI / 180
#define DELTA_ANGLE_Z 0.5f * PI / 180
#define ANGLE_SPEED 1.5f

const float _base_fov = 170.f;
const float _max_fov_add = 30.f;

CControllerPsyHitCamEffector::CControllerPsyHitCamEffector(ECamEffectorType type, const Fvector& src_pos,
    const Fvector& target_pos, float time, float base_fov, float dest_fov)
    : inherited(eCEControllerPsyHit, flt_max)
{
    m_base_fov = base_fov;
    m_dest_fov = dest_fov;
    m_time_total = time;
    m_time_current = 0;
    m_dangle_target.set(angle_normalize(Random.randFs(DELTA_ANGLE_X)), angle_normalize(Random.randFs(DELTA_ANGLE_Y)),
        angle_normalize(Random.randFs(DELTA_ANGLE_Z)));

    m_dangle_current.set(0.f, 0.f, 0.f);
    m_position_source = src_pos;
    m_direction.sub(target_pos, src_pos);
    m_distance = m_direction.magnitude();
    m_direction.normalize();
}

BOOL CControllerPsyHitCamEffector::ProcessCam(SCamEffectorInfo& info)
{
    Fmatrix Mdef;
    Mdef.identity();
    Mdef.j.set(info.n);
    Mdef.k.set(m_direction);
    Mdef.i.crossproduct(info.n, m_direction);
    Mdef.c.set(info.p);

    //////////////////////////////////////////////////////////////////////////

    if (angle_lerp(m_dangle_current.x, m_dangle_target.x, ANGLE_SPEED, Device.fTimeDelta))
    {
        m_dangle_target.x = angle_normalize(Random.randFs(DELTA_ANGLE_X));
    }

    if (angle_lerp(m_dangle_current.y, m_dangle_target.y, ANGLE_SPEED, Device.fTimeDelta))
    {
        m_dangle_target.y = angle_normalize(Random.randFs(DELTA_ANGLE_Y));
    }

    if (angle_lerp(m_dangle_current.z, m_dangle_target.z, ANGLE_SPEED, Device.fTimeDelta))
    {
        m_dangle_target.z = angle_normalize(Random.randFs(DELTA_ANGLE_Z));
    }

    //////////////////////////////////////////////////////////////////////////

    if (m_time_current > m_time_total)
        m_time_current = m_time_total;

    float perc_past = m_time_current / m_time_total;
    float cur_dist = m_distance * perc_past;

    Mdef.c.mad(m_position_source, m_direction, cur_dist);
    info.fFov = m_base_fov + (m_dest_fov - m_base_fov) * perc_past;
    // info.fFov = _base_fov - _max_fov_add*perc_past;

    m_time_current += Device.fTimeDelta;

    //////////////////////////////////////////////////////////////////////////

    // Установить углы смещения
    Fmatrix R;
    if (m_time_current > m_time_total)
        R.identity();
    else
        R.setHPB(m_dangle_current.x, m_dangle_current.y, m_dangle_current.z);

    Fmatrix mR;
    mR.mul(Mdef, R);

    info.d.set(mR.k);
    info.n.set(mR.j);
    info.p.set(mR.c);

    return TRUE;
}
bool similar_cam_info(const SCamEffectorInfo& c1, const SCamEffectorInfo& c2)
{
    return (c1.p.similar(c2.p, EPS_L) && c1.d.similar(c2.d, EPS_L) && c1.n.similar(c2.n, EPS_L) &&
        c1.r.similar(c2.r, EPS_L));
}
void CActorCameraManager::UpdateCamEffectors()
{
    m_cam_info_hud = m_cam_info;
    inherited::UpdateCamEffectors();

    m_cam_info_hud.d.normalize();
    m_cam_info_hud.n.normalize();
    m_cam_info_hud.r.crossproduct(m_cam_info_hud.n, m_cam_info_hud.d);
    m_cam_info_hud.n.crossproduct(m_cam_info_hud.d, m_cam_info_hud.r);
}

void cam_effector_sub(const SCamEffectorInfo& c1, const SCamEffectorInfo& c2, SCamEffectorInfo& dest)
{
    dest.p.sub(c1.p, c2.p);
    dest.d.sub(c1.d, c2.d);
    dest.n.sub(c1.n, c2.n);
    dest.r.sub(c1.r, c2.r);
}

void cam_effector_add(const SCamEffectorInfo& diff, SCamEffectorInfo& dest)
{
    dest.p.add(diff.p);
    dest.d.add(diff.d);
    dest.n.add(diff.n);
    dest.r.add(diff.r);
}

bool CActorCameraManager::ProcessCameraEffector(CEffectorCam* eff)
{
    SCamEffectorInfo prev = m_cam_info;

    bool res = inherited::ProcessCameraEffector(eff);
    if (res)
    {
        if (eff->GetHudAffect())
        {
            SCamEffectorInfo affected = m_cam_info;
            SCamEffectorInfo diff;

            cam_effector_sub(affected, prev, diff);

            cam_effector_add(diff, m_cam_info_hud); // m_cam_info_hud += difference
        }

        m_cam_info_hud.fFov = m_cam_info.fFov;
        m_cam_info_hud.fFar = m_cam_info.fFar;
        m_cam_info_hud.fAspect = m_cam_info.fAspect;
    }
    return res;
}
