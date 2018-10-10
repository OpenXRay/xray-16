#include "StdAfx.h"
#include "flare.h"
#include "player_hud.h"
#include "xrEngine/LightAnimLibrary.h"

void CFlare::Load(LPCSTR section)
{
    inherited::Load(section);
    m_work_time_sec = pSettings->r_float(section, "work_time_sec");
}

BOOL CFlare::net_Spawn(CSE_Abstract* DC)
{
    inherited::net_Spawn(DC);
    SwitchState(eFlareHidden);
    m_pFlareParticles = NULL;
    return TRUE;
}

void CFlare::net_Destroy() { inherited::net_Destroy(); }
void CFlare::UpdateXForm() {}
void CFlare::ActivateFlare()
{
    VERIFY(!IsFlareActive());
    SwitchState(eFlareShowing);
    SwitchOn();
}

bool CFlare::IsFlareActive()
{
    if (NULL == HudItemData())
        return false;

    VERIFY(g_player_hud->attached_item(HudItemData()->m_attach_place_idx) == HudItemData());

    return (GetState() == eFlareIdle);
}

void CFlare::OnStateSwitch(u32 S, u32 oldState)
{
    inherited::OnStateSwitch(S, oldState);

    switch (S)
    {
    case eFlareShowing:
    {
        g_player_hud->attach_item(this);
        PlayHUDMotion("anm_show", TRUE, this, GetState());
        SetPending(TRUE);
    }
    break;
    case eFlareHiding:
    {
        if (oldState != eFlareHiding)
        {
            PlayHUDMotion("anm_hide", TRUE, this, GetState());
            SetPending(TRUE);
        }
    }
    break;
    case eFlareIdle:
    {
        light_lanim = LALib.FindItem("flare_lanim_idle");
        SetPending(FALSE);
    }
    break;
    case eFlareHidden: { SetPending(FALSE);
    }
    break;
    case eFlareDropping:
    {
        PlayHUDMotion("anm_drop", TRUE, this, GetState());
        SetPending(TRUE);
    }
    break;
    };
}

void CFlare::OnAnimationEnd(u32 state)
{
    switch (state)
    {
    case eFlareShowing:
    {
        SwitchState(eFlareIdle);
        PlayAnimIdle();
    }
    break;
    case eFlareDropping:
    {
        SetDropManual(TRUE);
        SwitchState(eFlareHidden);
        processing_activate();
    }
    break;
    };
}

void CFlare::SwitchOn()
{
    static int lt = 1; // IRender_Light::POINT
    static bool ls = true;
    light_render = GEnv.Render->light_create();
    light_render->set_type((IRender_Light::LT)lt);
    light_render->set_shadow(ls);
    light_lanim = LALib.FindItem("flare_lanim_showing");

    light_render->set_active(true);

    m_pFlareParticles = CParticlesObject::Create(pSettings->r_string(cNameSect(), "working_particles"), FALSE);
    m_pFlareParticles->Play(true);
}

void CFlare::SwitchOff()
{
    SetCondition(0.0f);
    light_render.destroy();
    CParticlesObject::Destroy(m_pFlareParticles);
    processing_deactivate();
}

void CFlare::DropFlare()
{
    if (GetState() != eFlareHidden)
        SwitchState(eFlareDropping);
}

void CFlare::UpdateCL()
{
    inherited::UpdateCL();

    if (light_render /* && HudItemData()*/)
    {
        float _c = CurrStateTime() / 1000.0f;
        if (fsimilar(_c, m_work_time_sec) || _c > m_work_time_sec)
        {
            SwitchOff();
            return;
        }
        if (_c + 2.0f > m_work_time_sec) // 2sec
        {
            DropFlare();
        }

        int frame;
        Fcolor fclr;

        float fBrightness = 1.0f - powf(_c / m_work_time_sec, 4.0f);

        SetCondition(1.0f - _c / m_work_time_sec);

        u32 clr = light_lanim->CalculateBGR(Device.fTimeGlobal, frame);

        fclr.set((float)color_get_B(clr), (float)color_get_G(clr), (float)color_get_R(clr), 1.f);
        fclr.mul_rgb(fBrightness / 255.f);

        fclr.set(1, 1, 1, 1);

        light_render->set_color(fclr);

        Fvector _fp;
        FirePoint(_fp);
        light_render->set_position(_fp);

        Fmatrix pm;
        ParticlesMatrix(pm);

        m_pFlareParticles->UpdateParent(pm, Fvector().set(0.f, 0.f, 0.f));
    }
}

void CFlare::FirePoint(Fvector& _fp)
{
    if (HudItemData())
    {
        firedeps _current_firedeps;
        HudItemData()->setup_firedeps(_current_firedeps);
        _fp.set(_current_firedeps.vLastFP);
    }
    else
    {
        _fp.set(0, 0, 0);
        IKinematics* K = smart_cast<IKinematics*>(Visual());
        Fmatrix& fire_mat = K->LL_GetTransform(K->LL_BoneID("flare_point"));
        fire_mat.transform_tiny(_fp);
        XFORM().transform_tiny(_fp);
    }
}

void CFlare::ParticlesMatrix(Fmatrix& _pm)
{
    if (HudItemData())
    {
        firedeps _current_firedeps;
        HudItemData()->setup_firedeps(_current_firedeps);
        _pm.set(_current_firedeps.m_FireParticlesXForm);
        _pm.c.set(_current_firedeps.vLastFP);
    }
    else
    {
        Fvector _pd;
        _pd.set(0.f, 0.f, 1.f);
        XFORM().transform_dir(_pd);

        _pm.identity();
        _pm.k.set(_pd);
        Fvector::generate_orthonormal_basis_normalized(_pm.k, _pm.j, _pm.i);
        FirePoint(_pm.c);
    }
}
