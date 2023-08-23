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

SThunderboltDesc::SThunderboltDesc(const CInifile& pIni, shared_str const& sect)
{
    m_GradientTop    = create_gradient("gradient_top", pIni, sect);
    m_GradientCenter = create_gradient("gradient_center", pIni, sect);

    name = sect;
    color_anim = LALib.FindItem(pIni.r_string(sect, "color_anim"));
    VERIFY(color_anim);
    color_anim->fFPS = (float)color_anim->iFrameCount;

    // models
    m_pRender->CreateModel(pIni.r_string(sect, "lightning_model"));

    // sound
    cpcstr sound = pIni.r_string(sect, "sound");
    if (sound && sound[0])
        snd.create(sound, st_Effect, sg_Undefined);
}

SThunderboltDesc::SFlare* SThunderboltDesc::create_gradient(pcstr gradient_name, const CInifile& config, shared_str const& sect)
{
    string64 temp;
    return xr_new<SFlare>(
        config.r_float(sect, strconcat(temp, gradient_name, "_opacity")),
        config.r_fvector2(sect, strconcat(temp, gradient_name, "_radius")),
        config.r_string(sect, strconcat(temp, gradient_name, "_shader")),
        config.r_string(sect, strconcat(temp, gradient_name, "_texture"))
    );
}

SThunderboltDesc::~SThunderboltDesc()
{
    m_pRender->DestroyModel();

    snd.destroy();

    xr_delete(m_GradientTop);
    xr_delete(m_GradientCenter);
}

//----------------------------------------------------------------------------------------------
// collection
//----------------------------------------------------------------------------------------------
SThunderboltCollection::SThunderboltCollection(shared_str sect, CInifile const* pIni, CInifile const* thunderbolts)
{
    section = sect;
    const int tb_count = pIni->line_count(sect);
    palette.reserve(tb_count);
    for (int tb_idx = 0; tb_idx < tb_count; tb_idx++)
    {
        pcstr N, V;
        if (pIni->r_line(sect, tb_idx, &N, &V))
            palette.emplace_back(xr_new<SThunderboltDesc>(*thunderbolts, N));
    }
}

SThunderboltCollection::~SThunderboltCollection()
{
    for (auto& desc : palette)
        xr_delete(desc);
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

    const auto load_config = [](pcstr path) -> CInifile*
    {
        string_path filePath;
        if (FS.exist(filePath, "$game_config$", path))
            return xr_new<CInifile>(filePath, true, true, false);
        return nullptr;
    };

    m_thunderbolt_collections_config = load_config("environment\\thunderbolt_collections.ltx");
    m_thunderbolts_config = load_config("environment\\thunderbolts.ltx");

    pcstr section = "environment";
    CInifile const* config = load_config("environment\\environment.ltx");

    if (!config)
    {
        section = "thunderbolt_common";
        config = pSettings;
    }

    // params
    if (!config->try_read(p_var_alt, section, "altitude"))
    {
        p_var_alt.x = config->r_float(section, "altitude");
        p_var_alt.y = p_var_alt.x;
    }
    p_var_alt.x = deg2rad(p_var_alt.x);
    p_var_alt.y = deg2rad(p_var_alt.y);
    p_var_long = deg2rad(config->r_float(section, "delta_longitude"));
    p_min_dist = std::min(MAX_DIST_FACTOR, config->r_float(section, "min_dist_factor"));
    p_tilt = deg2rad(config->r_float(section, "tilt"));
    p_second_prop = config->r_float(section, "second_propability");
    clamp(p_second_prop, 0.f, 1.f);
    p_sky_color = config->r_float(section, "sky_color");
    p_sun_color = config->r_float(section, "sun_color");
    p_fog_color = config->r_float(section, "fog_color");

    if (config != pSettings)
        xr_delete(const_cast<CInifile *>(config));
}

CEffect_Thunderbolt::~CEffect_Thunderbolt()
{
    collections.clear();

    CInifile::Destroy(m_thunderbolt_collections_config);
    m_thunderbolt_collections_config = nullptr;

    CInifile::Destroy(m_thunderbolts_config);
    m_thunderbolts_config = nullptr;
}

SThunderboltCollection* CEffect_Thunderbolt::AppendDef(shared_str sect)
{
    if (!sect || (0 == sect[0]))
        return nullptr;

    for (const auto& item : collections)
        if (item->section == sect)
            return item;

    auto* result = xr_new<SThunderboltCollection>(sect,
        m_thunderbolt_collections_config ? m_thunderbolt_collections_config : pSettings,
        m_thunderbolts_config ? m_thunderbolts_config : pSettings
    );

    return collections.emplace_back(result);
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

void CEffect_Thunderbolt::Bolt(const CEnvDescriptorMixer& currentEnv)
{
    VERIFY(currentEnv.thunderbolt);
    state = stWorking;
    const float lt = currentEnv.bolt_duration;
    life_time = lt + Random.randF(-lt * 0.5f, lt * 0.5f);
    current_time = 0.f;

    current = currentEnv.thunderbolt->GetRandomDesc();
    VERIFY(current);

    float sun_h, sun_p;
    currentEnv.sun_dir.getHP(sun_h, sun_p);
    const auto far_dist = currentEnv.far_plane;
    const float period = currentEnv.bolt_period;

    Fmatrix XF, S;
    Fvector pos, dev;
    float alt = Random.randF(p_var_alt.x, p_var_alt.y);
    float lng = Random.randF(sun_h - p_var_long + PI, sun_h + p_var_long + PI);
    float dist = Random.randF(far_dist * p_min_dist, far_dist * .95f);
    current_direction.setHP(lng, alt);
    pos.mad(Device.vCameraPosition, current_direction, dist);
    dev.x = Random.randF(-p_tilt, p_tilt);
    dev.y = Random.randF(0, PI_MUL_2);
    dev.z = Random.randF(-p_tilt, p_tilt);
    XF.setXYZi(dev);

    Fvector light_dir = { 0.f, -1.f, 0.f };
    XF.transform_dir(light_dir);
    lightning_size = far_dist * 2.f;
    RayPick(pos, light_dir, lightning_size);

    lightning_center.mad(pos, light_dir, lightning_size * 0.5f);

    S.scale(lightning_size, lightning_size, lightning_size);
    XF.translate_over(pos);
    current_xform.mul_43(XF, S);

    float next_v = Random.randF();

    if (next_v < p_second_prop)
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

void CEffect_Thunderbolt::OnFrame(CEnvDescriptorMixer& currentEnv)
{
    const bool enabled = currentEnv.thunderbolt;
    if (bEnabled != enabled)
    {
        bEnabled = enabled;
        const float period = currentEnv.bolt_period;
        next_lightning_time = Device.fTimeGlobal + period + Random.randF(-period * 0.5f, period * 0.5f);
    }
    else if (bEnabled && (Device.fTimeGlobal > next_lightning_time))
    {
        if (state == stIdle && currentEnv.thunderbolt)
            Bolt(currentEnv);
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

        Fvector& sky_color = currentEnv.sky_color;
        sky_color.mad(fClr, p_sky_color);
        clamp(sky_color.x, 0.f, 1.f);
        clamp(sky_color.y, 0.f, 1.f);
        clamp(sky_color.z, 0.f, 1.f);

        currentEnv.sun_color.mad(fClr, p_sun_color);
        currentEnv.fog_color.mad(fClr, p_fog_color);

        if (GEnv.Render->GenerationIsR2OrHigher())
        {
            R_ASSERT(_valid(current_direction));
            currentEnv.sun_dir = current_direction;
            VERIFY2(currentEnv.sun_dir.y < 0,
                "Invalid sun direction settings while CEffect_Thunderbolt");
        }
    }
}

void CEffect_Thunderbolt::Render()
{
    if (state == stWorking)
        m_pRender->Render(*this);
}
