#include "stdafx.h"

#ifndef _EDITOR
#include "Render.h"
#endif
#include "thunderbolt.h"
#include "IGame_Persistent.h"
#include "LightAnimLibrary.h"

#ifdef _EDITOR
#include "ui_toolscustom.h"
#else
#include "IGame_Level.h"
#include "xrCDB/xr_area.h"
#include "xr_object.h"
#endif

SThunderboltDesc::~SThunderboltDesc()
{
    m_pRender->DestroyModel();

    m_GradientTop->m_pFlare->DestroyShader();
    m_GradientCenter->m_pFlare->DestroyShader();

    snd.destroy();

    xr_delete(m_GradientTop);
    xr_delete(m_GradientCenter);
}

void SThunderboltDesc::create_top_gradient(const CInifile& pIni, shared_str const& sect)
{
    m_GradientTop = xr_new<SFlare>();
    m_GradientTop->shader = pIni.r_string(sect, "gradient_top_shader");
    m_GradientTop->texture = pIni.r_string(sect, "gradient_top_texture");
    m_GradientTop->fRadius = pIni.r_fvector2(sect, "gradient_top_radius");
    m_GradientTop->fOpacity = pIni.r_float(sect, "gradient_top_opacity");
    m_GradientTop->m_pFlare->CreateShader(*m_GradientTop->shader, *m_GradientTop->texture);
}

void SThunderboltDesc::create_center_gradient(const CInifile& pIni, shared_str const& sect)
{
    m_GradientCenter = xr_new<SFlare>();
    m_GradientCenter->shader = pIni.r_string(sect, "gradient_center_shader");
    m_GradientCenter->texture = pIni.r_string(sect, "gradient_center_texture");
    m_GradientCenter->fRadius = pIni.r_fvector2(sect, "gradient_center_radius");
    m_GradientCenter->fOpacity = pIni.r_float(sect, "gradient_center_opacity");
    m_GradientCenter->m_pFlare->CreateShader(*m_GradientCenter->shader, *m_GradientCenter->texture);
}

void SThunderboltDesc::load(const CInifile& pIni, shared_str const& sect)
{
    create_top_gradient(pIni, sect);
    create_center_gradient(pIni, sect);

    name = sect;
    color_anim = LALib.FindItem(pIni.r_string(sect, "color_anim"));
    VERIFY(color_anim);
    color_anim->fFPS = (float)color_anim->iFrameCount;

    // models
    pcstr m_name = pIni.r_string(sect, "lightning_model");
    string_path tmp;
    xr_strcpy(tmp, m_name);
    m_pRender->CreateModel(tmp);

    // sound
    m_name = pIni.r_string(sect, "sound");
    xr_strcpy(tmp, m_name);
    if (m_name && m_name[0])
        snd.create(tmp, st_Effect, sg_Undefined);
}

//----------------------------------------------------------------------------------------------
// collection
//----------------------------------------------------------------------------------------------
void SThunderboltCollection::load(CInifile const* pIni, CInifile const* thunderbolts, pcstr sect)
{
    section = sect;
    int tb_count = pIni->line_count(sect);
    for (int tb_idx = 0; tb_idx < tb_count; tb_idx++)
    {
        pcstr N, V;
        if (pIni->r_line(sect, tb_idx, &N, &V))
            palette.push_back(g_pGamePersistent->Environment().thunderbolt_description(*thunderbolts, N));
    }
}
SThunderboltCollection::~SThunderboltCollection()
{
    for (auto d_it = palette.begin(); d_it != palette.end(); ++d_it)
        xr_delete(*d_it);

    palette.clear();
}

//----------------------------------------------------------------------------------------------
// thunderbolt effect
//----------------------------------------------------------------------------------------------
CEffect_Thunderbolt::CEffect_Thunderbolt()
{
    current = 0;
    life_time = 0.f;
    state = stIdle;
    next_lightning_time = 0.f;
    bEnabled = false;

    // params
    // p_var_alt = pSettings->r_fvector2 ( "environment","altitude" );
    // p_var_alt.x = deg2rad(p_var_alt.x); p_var_alt.y = deg2rad(p_var_alt.y);
    // p_var_long = deg2rad ( pSettings->r_float ( "environment","delta_longitude" ));
    // p_min_dist = _min (.95f,pSettings->r_float ( "environment","min_dist_factor" ));
    // p_tilt = deg2rad (pSettings->r_float ( "environment","tilt" ));
    // p_second_prop = pSettings->r_float ( "environment","second_propability" );
    // clamp (p_second_prop,0.f,1.f);
    // p_sky_color = pSettings->r_float ( "environment","sky_color" );
    // p_sun_color = pSettings->r_float ( "environment","sun_color" );
    // p_fog_color = pSettings->r_float ( "environment","fog_color" );
}

CEffect_Thunderbolt::~CEffect_Thunderbolt()
{
    for (auto d_it = collection.begin(); d_it != collection.end(); ++d_it)
        xr_delete(*d_it);

    collection.clear();
}

shared_str CEffect_Thunderbolt::AppendDef(
    CEnvironment& environment, CInifile const* pIni, CInifile const* thunderbolts, pcstr sect)
{
    if (!sect || (0 == sect[0]))
        return "";

    for (const auto item : collection)
        if (item->section == sect)
            return item->section;

    collection.push_back(environment.thunderbolt_collection(pIni, thunderbolts, sect));
    return collection.back()->section;
}

bool CEffect_Thunderbolt::RayPick(const Fvector& s, const Fvector& d, float& range)
{
    bool bRes = true;
#ifdef _EDITOR
    bRes = Tools->RayPick(s, d, range, 0, 0);
#else
    collide::rq_result RQ;
    IGameObject* E = g_pGameLevel->CurrentViewEntity();
    bRes = g_pGameLevel->ObjectSpace.RayPick(s, d, range, collide::rqtBoth, RQ, E);
    if (bRes)
        range = RQ.range;
    else
    {
        Fvector N = {0.f, -1.f, 0.f};
        Fvector P = {0.f, 0.f, 0.f};
        Fplane PL;
        PL.build(P, N);
        float dst = range;
        if (PL.intersectRayDist(s, d, dst) && (dst <= range))
        {
            range = dst;
            return true;
        }
        else
            return false;
    }
#endif
    return bRes;
}
#define FAR_DIST g_pGamePersistent->Environment().CurrentEnv->far_plane

void CEffect_Thunderbolt::Bolt(shared_str id, float period, float lt)
{
    VERIFY(id.size());
    state = stWorking;
    life_time = lt + Random.randF(-lt * 0.5f, lt * 0.5f);
    current_time = 0.f;

    current = g_pGamePersistent->Environment().thunderbolt_collection(collection, id)->GetRandomDesc();
    VERIFY(current);

    Fmatrix XF, S;
    Fvector pos, dev;
    float sun_h, sun_p;
    CEnvironment& environment = g_pGamePersistent->Environment();
    environment.CurrentEnv->sun_dir.getHP(sun_h, sun_p);
    float alt = Random.randF(environment.p_var_alt.x, environment.p_var_alt.y);
    float lng = Random.randF(sun_h - environment.p_var_long + PI, sun_h + environment.p_var_long + PI);
    float dist = Random.randF(FAR_DIST * environment.p_min_dist, FAR_DIST * .95f);
    current_direction.setHP(lng, alt);
    pos.mad(Device.vCameraPosition, current_direction, dist);
    dev.x = Random.randF(-environment.p_tilt, environment.p_tilt);
    dev.y = Random.randF(0, PI_MUL_2);
    dev.z = Random.randF(-environment.p_tilt, environment.p_tilt);
    XF.setXYZi(dev);

    Fvector light_dir = {0.f, -1.f, 0.f};
    XF.transform_dir(light_dir);
    lightning_size = FAR_DIST * 2.f;
    RayPick(pos, light_dir, lightning_size);

    lightning_center.mad(pos, light_dir, lightning_size * 0.5f);

    S.scale(lightning_size, lightning_size, lightning_size);
    XF.translate_over(pos);
    current_xform.mul_43(XF, S);

    float next_v = Random.randF();

    if (next_v < environment.p_second_prop)
    {
        next_lightning_time = Device.fTimeGlobal + lt + EPS_L;
    }
    else
    {
        next_lightning_time = Device.fTimeGlobal + period + Random.randF(-period * 0.3f, period * 0.3f);
        current->snd.play_no_feedback(0, 0, dist / 300.f, &pos, 0, 0, &Fvector2().set(dist / 2, dist * 2.f));
    }

    current_direction.invert(); // for env-sun
}

void CEffect_Thunderbolt::OnFrame(shared_str id, float period, float duration)
{
    bool enabled = !!(id.size());
    if (bEnabled != enabled)
    {
        bEnabled = enabled;
        next_lightning_time = Device.fTimeGlobal + period + Random.randF(-period * 0.5f, period * 0.5f);
    }
    else if (bEnabled && (Device.fTimeGlobal > next_lightning_time))
    {
        if (state == stIdle && !!(id.size()))
            Bolt(id, period, duration);
    }
    if (state == stWorking)
    {
        if (current_time > life_time)
            state = stIdle;
        current_time += Device.fTimeDelta;
        Fvector fClr;
        int frame;
        u32 uClr = current->color_anim->CalculateRGB(current_time / life_time, frame);
        fClr.set(clampr(float(color_get_R(uClr) / 255.f), 0.f, 1.f), clampr(float(color_get_G(uClr) / 255.f), 0.f, 1.f),
            clampr(float(color_get_B(uClr) / 255.f), 0.f, 1.f));

        lightning_phase = 1.5f * (current_time / life_time);
        clamp(lightning_phase, 0.f, 1.f);

        CEnvironment& environment = g_pGamePersistent->Environment();

        Fvector& sky_color = environment.CurrentEnv->sky_color;
        sky_color.mad(fClr, environment.p_sky_color);
        clamp(sky_color.x, 0.f, 1.f);
        clamp(sky_color.y, 0.f, 1.f);
        clamp(sky_color.z, 0.f, 1.f);

        environment.CurrentEnv->sun_color.mad(fClr, environment.p_sun_color);
        environment.CurrentEnv->fog_color.mad(fClr, environment.p_fog_color);

        if (GEnv.Render->get_generation() == IRender::GENERATION_R2)
        {
            R_ASSERT(_valid(current_direction));
            g_pGamePersistent->Environment().CurrentEnv->sun_dir = current_direction;
            VERIFY2(g_pGamePersistent->Environment().CurrentEnv->sun_dir.y < 0,
                "Invalid sun direction settings while CEffect_Thunderbolt");
        }
    }
}

void CEffect_Thunderbolt::Render()
{
    if (state == stWorking)
        m_pRender->Render(*this);
}
