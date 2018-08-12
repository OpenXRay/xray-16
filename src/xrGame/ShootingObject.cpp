//////////////////////////////////////////////////////////////////////
// ShootingObject.cpp:  интерфейс для семейства стреляющих объектов
//						(оружие и осколочные гранаты)
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "ShootingObject.h"

#include "ParticlesObject.h"
#include "WeaponAmmo.h"

#include "Actor.h"
#include "Spectator.h"
#include "game_cl_base.h"
#include "Level.h"
#include "Level_Bullet_Manager.h"
#include "game_cl_single.h"

#define HIT_POWER_EPSILON 0.05f
#define WALLMARK_SIZE 0.04f

CShootingObject::CShootingObject(void)
{
    fShotTimeCounter = 0;
    fOneShotTime = 0;
    // fHitPower						= 0.0f;
    fvHitPower.set(0.0f, 0.0f, 0.0f, 0.0f);
    fvHitPowerCritical.set(0.0f, 0.0f, 0.0f, 0.0f);
    m_fStartBulletSpeed = 1000.f;

    m_vCurrentShootDir.set(0, 0, 0);
    m_vCurrentShootPos.set(0, 0, 0);
    m_iCurrentParentID = 0xFFFF;

    m_fPredBulletTime = 0.0f;
    m_bUseAimBullet = false;
    m_fTimeToAim = 0.0f;

    // particles
    m_sFlameParticlesCurrent = m_sFlameParticles = NULL;
    m_sSmokeParticlesCurrent = m_sSmokeParticles = NULL;
    m_sShellParticles = NULL;

    bWorking = false;

    light_render = 0;

    reinit();
}
CShootingObject::~CShootingObject(void) {}
void CShootingObject::reinit() { m_pFlameParticles = NULL; }
void CShootingObject::Load(LPCSTR section)
{
    if (pSettings->line_exist(section, "light_disabled"))
    {
        m_bLightShotEnabled = !pSettings->r_bool(section, "light_disabled");
    }
    else
        m_bLightShotEnabled = true;

    //время затрачиваемое на выстрел
    fOneShotTime = pSettings->r_float(section, "rpm");
    //Alundaio: Two-shot burst rpm; used for Abakan/AN-94
    modeShotTime = READ_IF_EXISTS(pSettings, r_float, section, "rpm_mode_2", fOneShotTime);

    VERIFY(fOneShotTime > 0.f);
    fOneShotTime = 60.f / fOneShotTime;
    modeShotTime = 60.f / modeShotTime;

    //Cycle down RPM after first 2 shots; used for Abakan/AN-94
    if (pSettings->line_exist(section, "cycle_down"))
        cycleDown = pSettings->r_bool(section, "cycle_down") ? true : false;
    else
        cycleDown = false;
    //Alundaio: END

    LoadFireParams(section);
    LoadLights(section, "");
    LoadShellParticles(section, "");
    LoadFlameParticles(section, "");

    m_air_resistance_factor = READ_IF_EXISTS(pSettings, r_float, section, "air_resistance_factor", 1.f);
}

void CShootingObject::Light_Create()
{
    // lights
    light_render = GEnv.Render->light_create();
    if (GEnv.Render->get_generation() == IRender::GENERATION_R2)
        light_render->set_shadow(true);
    else
        light_render->set_shadow(false);
}

void CShootingObject::Light_Destroy() { light_render.destroy(); }
void CShootingObject::LoadFireParams(LPCSTR section)
{
    string32 buffer;
    shared_str s_sHitPower;
    shared_str s_sHitPowerCritical;

    //базовая дисперсия оружия
    fireDispersionBase = deg2rad(pSettings->r_float(section, "fire_dispersion_base"));

    //сила выстрела и его мощьность
    s_sHitPower = pSettings->r_string_wb(section, "hit_power"); //читаем строку силы хита пули оружия
    s_sHitPowerCritical = pSettings->r_string_wb(section, "hit_power_critical");
    fvHitPower[egdMaster] =
        (float)atof(_GetItem(*s_sHitPower, 0, buffer)); //первый параметр - это хит для уровня игры мастер
    fvHitPowerCritical[egdMaster] =
        (float)atof(_GetItem(*s_sHitPowerCritical, 0, buffer)); //первый параметр - это хит для уровня игры мастер

    fvHitPower[egdNovice] = fvHitPower[egdStalker] = fvHitPower[egdVeteran] =
        fvHitPower[egdMaster]; //изначально параметры для других уровней сложности такие же
    fvHitPowerCritical[egdNovice] = fvHitPowerCritical[egdStalker] = fvHitPowerCritical[egdVeteran] =
        fvHitPowerCritical[egdMaster]; //изначально параметры для других уровней сложности такие же

    int num_game_diff_param = _GetItemCount(*s_sHitPower); //узнаём колличество параметров для хитов
    if (num_game_diff_param > 1) //если задан второй параметр хита
    {
        fvHitPower[egdVeteran] = (float)atof(_GetItem(*s_sHitPower, 1, buffer)); //то вычитываем его для уровня ветерана
    }
    if (num_game_diff_param > 2) //если задан третий параметр хита
    {
        fvHitPower[egdStalker] = (float)atof(_GetItem(*s_sHitPower, 2, buffer)); //то вычитываем его для уровня сталкера
    }
    if (num_game_diff_param > 3) //если задан четвёртый параметр хита
    {
        fvHitPower[egdNovice] = (float)atof(_GetItem(*s_sHitPower, 3, buffer)); //то вычитываем его для уровня новичка
    }

    num_game_diff_param = _GetItemCount(*s_sHitPowerCritical); //узнаём колличество параметров
    if (num_game_diff_param > 1) //если задан второй параметр хита
    {
        fvHitPowerCritical[egdVeteran] =
            (float)atof(_GetItem(*s_sHitPowerCritical, 1, buffer)); //то вычитываем его для уровня ветерана
    }
    if (num_game_diff_param > 2) //если задан третий параметр хита
    {
        fvHitPowerCritical[egdStalker] =
            (float)atof(_GetItem(*s_sHitPowerCritical, 2, buffer)); //то вычитываем его для уровня сталкера
    }
    if (num_game_diff_param > 3) //если задан четвёртый параметр хита
    {
        fvHitPowerCritical[egdNovice] =
            (float)atof(_GetItem(*s_sHitPowerCritical, 3, buffer)); //то вычитываем его для уровня новичка
    }

    fHitImpulse = pSettings->r_float(section, "hit_impulse");
    //максимальное расстояние полета пули
    fireDistance = pSettings->r_float(section, "fire_distance");
    //начальная скорость пули
    m_fStartBulletSpeed = pSettings->r_float(section, "bullet_speed");
    m_bUseAimBullet = pSettings->r_bool(section, "use_aim_bullet");
    if (m_bUseAimBullet)
    {
        m_fTimeToAim = pSettings->r_float(section, "time_to_aim");
    }
}

void CShootingObject::LoadLights(LPCSTR section, LPCSTR prefix)
{
    string256 full_name;
    // light
    if (m_bLightShotEnabled)
    {
        Fvector clr = pSettings->r_fvector3(section, strconcat(sizeof(full_name), full_name, prefix, "light_color"));
        light_base_color.set(clr.x, clr.y, clr.z, 1);
        light_base_range = pSettings->r_float(section, strconcat(sizeof(full_name), full_name, prefix, "light_range"));
        light_var_color =
            pSettings->r_float(section, strconcat(sizeof(full_name), full_name, prefix, "light_var_color"));
        light_var_range =
            pSettings->r_float(section, strconcat(sizeof(full_name), full_name, prefix, "light_var_range"));
        light_lifetime = pSettings->r_float(section, strconcat(sizeof(full_name), full_name, prefix, "light_time"));
        light_time = -1.f;
    }
}

void CShootingObject::Light_Start()
{
    if (!light_render)
        Light_Create();

    if (Device.dwFrame != light_frame)
    {
        light_frame = Device.dwFrame;
        light_time = light_lifetime;

        light_build_color.set(Random.randFs(light_var_color, light_base_color.r),
            Random.randFs(light_var_color, light_base_color.g), Random.randFs(light_var_color, light_base_color.b), 1);
        light_build_range = Random.randFs(light_var_range, light_base_range);
    }
}

void CShootingObject::Light_Render(const Fvector& P)
{
    float light_scale = light_time / light_lifetime;
    R_ASSERT(light_render);

    light_render->set_position(P);
    light_render->set_color(
        light_build_color.r * light_scale, light_build_color.g * light_scale, light_build_color.b * light_scale);
    light_render->set_range(light_build_range * light_scale);

    if (!light_render->get_active())
    {
        light_render->set_active(true);
    }
}

//////////////////////////////////////////////////////////////////////////
// Particles
//////////////////////////////////////////////////////////////////////////

void CShootingObject::StartParticles(
    CParticlesObject*& pParticles, LPCSTR particles_name, const Fvector& pos, const Fvector& vel, bool auto_remove_flag)
{
    if (!particles_name)
        return;

    if (pParticles != NULL)
    {
        UpdateParticles(pParticles, pos, vel);
        return;
    }

    pParticles = CParticlesObject::Create(particles_name, (BOOL)auto_remove_flag);

    UpdateParticles(pParticles, pos, vel);
    CSpectator* tmp_spectr = smart_cast<CSpectator*>(Level().CurrentControlEntity());
    bool in_hud_mode = IsHudModeNow();
    if (in_hud_mode && tmp_spectr && (tmp_spectr->GetActiveCam() != CSpectator::eacFirstEye))
    {
        in_hud_mode = false;
    }
    pParticles->Play(in_hud_mode);
}
void CShootingObject::StopParticles(CParticlesObject*& pParticles)
{
    if (pParticles == NULL)
        return;

    pParticles->Stop();
    CParticlesObject::Destroy(pParticles);
}

void CShootingObject::UpdateParticles(CParticlesObject*& pParticles, const Fvector& pos, const Fvector& vel)
{
    if (!pParticles)
        return;

    Fmatrix particles_pos;
    particles_pos.set(get_ParticlesXFORM());
    particles_pos.c.set(pos);

    pParticles->SetXFORM(particles_pos);

    if (!pParticles->IsAutoRemove() && !pParticles->IsLooped() && !pParticles->PSI_alive())
    {
        pParticles->Stop();
        CParticlesObject::Destroy(pParticles);
    }
}

void CShootingObject::LoadShellParticles(LPCSTR section, LPCSTR prefix)
{
    string256 full_name;
    strconcat(sizeof(full_name), full_name, prefix, "shell_particles");

    if (pSettings->line_exist(section, full_name))
    {
        m_sShellParticles = pSettings->r_string(section, full_name);
        vLoadedShellPoint =
            pSettings->r_fvector3(section, strconcat(sizeof(full_name), full_name, prefix, "shell_point"));
    }
}

void CShootingObject::LoadFlameParticles(LPCSTR section, LPCSTR prefix)
{
    string256 full_name;

    // flames
    strconcat(sizeof(full_name), full_name, prefix, "flame_particles");
    if (pSettings->line_exist(section, full_name))
        m_sFlameParticles = pSettings->r_string(section, full_name);

    strconcat(sizeof(full_name), full_name, prefix, "smoke_particles");
    if (pSettings->line_exist(section, full_name))
        m_sSmokeParticles = pSettings->r_string(section, full_name);

    strconcat(sizeof(full_name), full_name, prefix, "shot_particles");
    if (pSettings->line_exist(section, full_name))
        m_sShotParticles = pSettings->r_string(section, full_name);

    //текущие партиклы
    m_sFlameParticlesCurrent = m_sFlameParticles;
    m_sSmokeParticlesCurrent = m_sSmokeParticles;
}

void CShootingObject::OnShellDrop(const Fvector& play_pos, const Fvector& parent_vel)
{
    if (!m_sShellParticles)
        return;
    if (Device.vCameraPosition.distance_to_sqr(play_pos) > 2 * 2)
        return;

    CParticlesObject* pShellParticles = CParticlesObject::Create(*m_sShellParticles, TRUE);

    Fmatrix particles_pos;
    particles_pos.set(get_ParticlesXFORM());
    particles_pos.c.set(play_pos);

    pShellParticles->UpdateParent(particles_pos, parent_vel);
    CSpectator* tmp_spectr = smart_cast<CSpectator*>(Level().CurrentControlEntity());
    bool in_hud_mode = IsHudModeNow();
    if (in_hud_mode && tmp_spectr && (tmp_spectr->GetActiveCam() != CSpectator::eacFirstEye))
    {
        in_hud_mode = false;
    }
    pShellParticles->Play(in_hud_mode);
}

//партиклы дыма
void CShootingObject::StartSmokeParticles(const Fvector& play_pos, const Fvector& parent_vel)
{
    CParticlesObject* pSmokeParticles = NULL;
    StartParticles(pSmokeParticles, *m_sSmokeParticlesCurrent, play_pos, parent_vel, true);
}

void CShootingObject::StartFlameParticles()
{
    if (0 == m_sFlameParticlesCurrent.size())
        return;

    //если партиклы циклические
    if (m_pFlameParticles && m_pFlameParticles->IsLooped() && m_pFlameParticles->IsPlaying())
    {
        UpdateFlameParticles();
        return;
    }

    StopFlameParticles();
    m_pFlameParticles = CParticlesObject::Create(*m_sFlameParticlesCurrent, FALSE);
    UpdateFlameParticles();

    CSpectator* tmp_spectr = smart_cast<CSpectator*>(Level().CurrentControlEntity());
    bool in_hud_mode = IsHudModeNow();
    if (in_hud_mode && tmp_spectr && (tmp_spectr->GetActiveCam() != CSpectator::eacFirstEye))
    {
        in_hud_mode = false;
    }
    m_pFlameParticles->Play(in_hud_mode);
}
void CShootingObject::StopFlameParticles()
{
    if (0 == m_sFlameParticlesCurrent.size())
        return;
    if (m_pFlameParticles == NULL)
        return;

    m_pFlameParticles->SetAutoRemove(true);
    m_pFlameParticles->Stop();
    m_pFlameParticles = NULL;
}

void CShootingObject::UpdateFlameParticles()
{
    if (0 == m_sFlameParticlesCurrent.size())
        return;
    if (!m_pFlameParticles)
        return;

    Fmatrix pos;
    pos.set(get_ParticlesXFORM());
    pos.c.set(get_CurrentFirePoint());

    VERIFY(_valid(pos));

    m_pFlameParticles->SetXFORM(pos);

    if (!m_pFlameParticles->IsLooped() && !m_pFlameParticles->IsPlaying() && !m_pFlameParticles->PSI_alive())
    {
        m_pFlameParticles->Stop();
        CParticlesObject::Destroy(m_pFlameParticles);
    }
}

//подсветка от выстрела
void CShootingObject::UpdateLight()
{
    if (light_render && light_time > 0)
    {
        light_time -= Device.fTimeDelta;
        if (light_time <= 0)
            StopLight();
    }
}

void CShootingObject::StopLight()
{
    if (light_render)
    {
        light_render->set_active(false);
    }
}

void CShootingObject::RenderLight()
{
    if (light_render && light_time > 0)
    {
        Light_Render(get_CurrentFirePoint());
    }
}

bool CShootingObject::SendHitAllowed(IGameObject* pUser)
{
    if (Game().IsServerControlHits())
        return OnServer();

    if (OnServer())
    {
        if (smart_cast<CActor*>(pUser))
        {
            if (Level().CurrentControlEntity() != pUser)
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        if (smart_cast<CActor*>(pUser))
        {
            if (Level().CurrentControlEntity() == pUser)
            {
                return true;
            }
        }
        return false;
    }
};

extern void random_dir(Fvector& tgt_dir, const Fvector& src_dir, float dispersion);

void CShootingObject::FireBullet(const Fvector& pos, const Fvector& shot_dir, float fire_disp,
    const CCartridge& cartridge, u16 parent_id, u16 weapon_id, bool send_hit)
{
    Fvector dir;
    random_dir(dir, shot_dir, fire_disp);

    m_vCurrentShootDir = dir;
    m_vCurrentShootPos = pos;
    m_iCurrentParentID = parent_id;

    bool aim_bullet;
    if (m_bUseAimBullet)
    {
        if (ParentMayHaveAimBullet())
        {
            if (m_fPredBulletTime == 0.0)
            {
                aim_bullet = true;
            }
            else
            {
                if ((Device.fTimeGlobal - m_fPredBulletTime) >= m_fTimeToAim)
                {
                    aim_bullet = true;
                }
                else
                {
                    aim_bullet = false;
                }
            }
        }
        else
        {
            aim_bullet = false;
        }
    }
    else
    {
        aim_bullet = false;
    }
    m_fPredBulletTime = Device.fTimeGlobal;

    float l_fHitPower = 0.0f;
    if (ParentIsActor()) //если из оружия стреляет актёр(игрок)
    {
        if (GameID() == eGameIDSingle)
        {
            l_fHitPower = fvHitPower[g_SingleGameDifficulty];
        }
        else
        {
            l_fHitPower = fvHitPower[egdMaster];
        }
    }
    else
    {
        l_fHitPower = fvHitPower[egdMaster];
    }

    Level().BulletManager().AddBullet(pos, dir, m_fStartBulletSpeed * cur_silencer_koef.bullet_speed,
        l_fHitPower * cur_silencer_koef.hit_power, fHitImpulse * cur_silencer_koef.hit_impulse, parent_id, weapon_id,
        ALife::eHitTypeFireWound, fireDistance, cartridge, m_air_resistance_factor, send_hit, aim_bullet);
}
void CShootingObject::FireStart() { bWorking = true; }
void CShootingObject::FireEnd() { bWorking = false; }
void CShootingObject::StartShotParticles()
{
    CParticlesObject* pSmokeParticles = NULL;
    StartParticles(pSmokeParticles, *m_sShotParticles, m_vCurrentShootPos, m_vCurrentShootDir, true);
}
