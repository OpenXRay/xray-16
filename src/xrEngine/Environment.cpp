#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#include "Render.h"
#endif

#include "Environment.h"
#include "xr_efflensflare.h"
#include "Rain.h"
#include "thunderbolt.h"
#include "xrHemisphere.h"
#include "perlin.h"

#ifndef _EDITOR
#include "IGame_Level.h"
#endif

#include "xrCore/xrCore.h"

#include "Include/xrRender/EnvironmentRender.h"
#include "Include/xrRender/LensFlareRender.h"
#include "Include/xrRender/RainRender.h"
#include "Include/xrRender/ThunderboltRender.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
ENGINE_API float psVisDistance = 1.f;
static const float MAX_NOISE_FREQ = 0.03f;

//#define WEATHER_LOGGING

// real WEATHER->WFX transition time
#define WFX_TRANS_TIME 5.f

const float MAX_DIST_FACTOR = 0.95f;

//////////////////////////////////////////////////////////////////////////
// environment
CEnvironment::CEnvironment() : CurrentEnv(0), m_ambients_config(0)
{
    bNeed_re_create_env = FALSE;
    bWFX = false;
    Current[0] = 0;
    Current[1] = 0;
    CurrentWeather = 0;
    CurrentWeatherName = 0;
    eff_Rain = 0;
    eff_LensFlare = 0;
    eff_Thunderbolt = 0;
    OnDeviceCreate();
#ifdef _EDITOR
    ed_from_time = 0.f;
    ed_to_time = DAY_LENGTH;
#endif

#ifndef _EDITOR
    m_paused = false;
#endif

    fGameTime = 0.f;
    fTimeFactor = 12.f;

    wind_strength_factor = 0.f;
    wind_gust_factor = 0.f;

    wind_blast_strength = 0.f;
    wind_blast_direction.set(1.f, 0.f, 0.f);

    wind_blast_strength_start_value = 0.f;
    wind_blast_strength_stop_value = 0.f;

    // fill clouds hemi verts & faces
    const Fvector* verts;
    CloudsVerts.resize(xrHemisphereVertices(2, verts));
    CopyMemory(&CloudsVerts.front(), verts, CloudsVerts.size() * sizeof(Fvector));
    const u16* indices;
    CloudsIndices.resize(xrHemisphereIndices(2, indices));
    CopyMemory(&CloudsIndices.front(), indices, CloudsIndices.size() * sizeof(u16));

    // perlin noise
    PerlinNoise1D = new CPerlinNoise1D(Random.randI(0, 0xFFFF));
    PerlinNoise1D->SetOctaves(2);
    PerlinNoise1D->SetAmplitude(0.66666f);

    // tsky0 = Device.Resources->_CreateTexture("$user$sky0");
    // tsky1 = Device.Resources->_CreateTexture("$user$sky1");

    string_path file_name;
    m_ambients_config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "ambients.ltx"), TRUE, TRUE, FALSE);
    m_sound_channels_config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "sound_channels.ltx"), TRUE, TRUE, FALSE);
    m_effects_config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "effects.ltx"), TRUE, TRUE, FALSE);
    m_suns_config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "suns.ltx"), TRUE, TRUE, FALSE);
    m_thunderbolt_collections_config = new CInifile(
        FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolt_collections.ltx"), TRUE, TRUE, FALSE);
    m_thunderbolts_config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolts.ltx"), TRUE, TRUE, FALSE);

    CInifile* config =
        new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "environment.ltx"), TRUE, TRUE, FALSE);

    // params
    p_var_alt = deg2rad(config->r_float("environment", "altitude"));
    p_var_long = deg2rad(config->r_float("environment", "delta_longitude"));
    p_min_dist = std::min(.95f, config->r_float("environment", "min_dist_factor"));
    p_tilt = deg2rad(config->r_float("environment", "tilt"));
    p_second_prop = config->r_float("environment", "second_propability");
    clamp(p_second_prop, 0.f, 1.f);
    p_sky_color = config->r_float("environment", "sky_color");
    p_sun_color = config->r_float("environment", "sun_color");
    p_fog_color = config->r_float("environment", "fog_color");

    xr_delete(config);

    // OpenXRay environment configuration
    useDynamicSunDir = pSettingsOpenXRay->read_if_exists<bool>("environment", "dynamic_sun_dir", true);
    sunDirAzimuth = pSettingsOpenXRay->read_if_exists<float>("environment", "sun_dir_azimuth", 0.0f);
    clamp(sunDirAzimuth, 0.0f, 360.0f);
    sunDirAzimuth *= (PI / 180.0f);
}

CEnvironment::~CEnvironment()
{
    xr_delete(PerlinNoise1D);
    OnDeviceDestroy();

    VERIFY(m_ambients_config);
    CInifile::Destroy(m_ambients_config);
    m_ambients_config = 0;

    VERIFY(m_sound_channels_config);
    CInifile::Destroy(m_sound_channels_config);
    m_sound_channels_config = 0;

    VERIFY(m_effects_config);
    CInifile::Destroy(m_effects_config);
    m_effects_config = 0;

    VERIFY(m_suns_config);
    CInifile::Destroy(m_suns_config);
    m_suns_config = 0;

    VERIFY(m_thunderbolt_collections_config);
    CInifile::Destroy(m_thunderbolt_collections_config);
    m_thunderbolt_collections_config = 0;

    VERIFY(m_thunderbolts_config);
    CInifile::Destroy(m_thunderbolts_config);
    m_thunderbolts_config = 0;

    destroy_mixer();
}

void CEnvironment::Invalidate()
{
    bWFX = false;
    Current[0] = 0;
    Current[1] = 0;
    if (eff_LensFlare)
        eff_LensFlare->Invalidate();
}

float CEnvironment::TimeDiff(float prev, float cur)
{
    if (prev > cur)
        return (DAY_LENGTH - prev) + cur;
    else
        return cur - prev;
}

float CEnvironment::TimeWeight(float val, float min_t, float max_t)
{
    float weight = 0.f;
    float length = TimeDiff(min_t, max_t);
    if (!fis_zero(length, EPS))
    {
        if (min_t > max_t)
        {
            if ((val >= min_t) || (val <= max_t))
                weight = TimeDiff(min_t, val) / length;
        }
        else
        {
            if ((val >= min_t) && (val <= max_t))
                weight = TimeDiff(min_t, val) / length;
        }
        clamp(weight, 0.f, 1.f);
    }
    return weight;
}
void CEnvironment::ChangeGameTime(float game_time) { fGameTime = NormalizeTime(fGameTime + game_time); };
void CEnvironment::SetGameTime(float game_time, float time_factor)
{
#ifndef _EDITOR
    if (m_paused) // BUG nitrocaster: g_pGameLevel may be null (game not started) -> crash
    {
        g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(fGameTime * 1000.f), fTimeFactor);
        return;
    }
#endif
    if (bWFX)
        wfx_time -= TimeDiff(fGameTime, game_time);
    fGameTime = game_time;
    fTimeFactor = time_factor;
}

float CEnvironment::NormalizeTime(float tm)
{
    if (tm < 0.f)
        return tm + DAY_LENGTH;
    else if (tm > DAY_LENGTH)
        return tm - DAY_LENGTH;
    else
        return tm;
}

void CEnvironment::SetWeather(shared_str name, bool forced)
{
    //. static BOOL bAlready = FALSE;
    //. if(bAlready) return;
    if (name.size())
    {
        //. bAlready = TRUE;
        auto it = WeatherCycles.find(name);
        if (it == WeatherCycles.end())
        {
            Msg("! Invalid weather name: %s", name.c_str());
            return;
        }
        R_ASSERT3(it != WeatherCycles.end(), "Invalid weather name.", *name);
        CurrentCycleName = it->first;
        if (forced)
        {
            Invalidate();
        }
        if (!bWFX)
        {
            CurrentWeather = &it->second;
            CurrentWeatherName = it->first;
        }
        if (forced)
        {
            SelectEnvs(fGameTime);
        }
#ifdef WEATHER_LOGGING
        Msg("Starting Cycle: %s [%s]", *name, forced ? "forced" : "deferred");
#endif
    }
    else
    {
#ifndef _EDITOR
        FATAL("! Empty weather name");
#endif
    }
}

bool CEnvironment::SetWeatherFX(shared_str name)
{
    if (bWFX)
        return false;
    if (name.size())
    {
        auto it = WeatherFXs.find(name);
        R_ASSERT3(it != WeatherFXs.end(), "Invalid weather effect name.", *name);
        EnvVec* PrevWeather = CurrentWeather;
        VERIFY(PrevWeather);
        CurrentWeather = &it->second;
        CurrentWeatherName = it->first;

        float rewind_tm = WFX_TRANS_TIME * fTimeFactor;
        float start_tm = fGameTime + rewind_tm;
        float current_length;
        float current_weight;
        if (Current[0]->exec_time > Current[1]->exec_time)
        {
            float x = fGameTime > Current[0]->exec_time ? fGameTime - Current[0]->exec_time :
                                                          (DAY_LENGTH - Current[0]->exec_time) + fGameTime;
            current_length = (DAY_LENGTH - Current[0]->exec_time) + Current[1]->exec_time;
            current_weight = x / current_length;
        }
        else
        {
            current_length = Current[1]->exec_time - Current[0]->exec_time;
            current_weight = (fGameTime - Current[0]->exec_time) / current_length;
        }
        clamp(current_weight, 0.f, 1.f);

        std::sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_etl_pred);
        CEnvDescriptor* C0 = CurrentWeather->at(0);
        CEnvDescriptor* C1 = CurrentWeather->at(1);
        CEnvDescriptor* CE = CurrentWeather->at(CurrentWeather->size() - 2);
        CEnvDescriptor* CT = CurrentWeather->at(CurrentWeather->size() - 1);
        C0->copy(*Current[0]);
        C0->exec_time =
            NormalizeTime(fGameTime - ((rewind_tm / (Current[1]->exec_time - fGameTime)) * current_length - rewind_tm));
        C1->copy(*Current[1]);
        C1->exec_time = NormalizeTime(start_tm);
        for (auto t_it = CurrentWeather->begin() + 2; t_it != CurrentWeather->end() - 1; t_it++)
            (*t_it)->exec_time = NormalizeTime(start_tm + (*t_it)->exec_time_loaded);
        SelectEnv(PrevWeather, WFX_end_desc[0], CE->exec_time);
        SelectEnv(PrevWeather, WFX_end_desc[1], WFX_end_desc[0]->exec_time + 0.5f);
        CT->copy(*WFX_end_desc[0]);
        CT->exec_time = NormalizeTime(CE->exec_time + rewind_tm);
        wfx_time = TimeDiff(fGameTime, CT->exec_time);
        bWFX = true;

        // sort wfx envs
        std::sort(CurrentWeather->begin(), CurrentWeather->end(), sort_env_pred);

        Current[0] = C0;
        Current[1] = C1;
#ifdef WEATHER_LOGGING
        Msg("Starting WFX: '%s' - %3.2f sec", *name, wfx_time);
// for (auto l_it=CurrentWeather->begin(); l_it!=CurrentWeather->end(); l_it++)
// Msg (". Env: '%s' Tm: %3.2f",*(*l_it)->m_identifier.c_str(),(*l_it)->exec_time);
#endif
    }
    else
    {
#ifndef _EDITOR
        FATAL("! Empty weather effect name");
#endif
    }
    return true;
}

bool CEnvironment::StartWeatherFXFromTime(shared_str name, float time)
{
    if (!SetWeatherFX(name))
        return false;

    for (auto it = CurrentWeather->begin(); it != CurrentWeather->end(); it++)
        (*it)->exec_time = NormalizeTime((*it)->exec_time - wfx_time + time);

    wfx_time = time;
    return true;
}

void CEnvironment::StopWFX()
{
    VERIFY(CurrentCycleName.size());
    bWFX = false;
    SetWeather(CurrentCycleName, false);
    Current[0] = WFX_end_desc[0];
    Current[1] = WFX_end_desc[1];
#ifdef WEATHER_LOGGING
    Msg("WFX - end. Weather: '%s' Desc: '%s'/'%s' GameTime: %3.2f", CurrentWeatherName.c_str(),
        Current[0]->m_identifier.c_str(), Current[1]->m_identifier.c_str(), fGameTime);
#endif
}

IC bool lb_env_pred(const CEnvDescriptor* x, float val) { return x->exec_time < val; }
void CEnvironment::SelectEnv(EnvVec* envs, CEnvDescriptor*& e, float gt)
{
    auto env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);
    if (env == envs->end())
    {
        e = envs->front();
    }
    else
    {
        e = *env;
    }
}

void CEnvironment::SelectEnvs(EnvVec* envs, CEnvDescriptor*& e0, CEnvDescriptor*& e1, float gt)
{
    auto env = std::lower_bound(envs->begin(), envs->end(), gt, lb_env_pred);
    if (env == envs->end())
    {
        e0 = *(envs->end() - 1);
        e1 = envs->front();
    }
    else
    {
        e1 = *env;
        if (env == envs->begin())
            e0 = *(envs->end() - 1);
        else
            e0 = *(env - 1);
    }
}

void CEnvironment::SelectEnvs(float gt)
{
    VERIFY(CurrentWeather);
    if ((Current[0] == Current[1]) && (Current[0] == 0))
    {
        VERIFY(!bWFX);
        // first or forced start
        SelectEnvs(CurrentWeather, Current[0], Current[1], gt);
    }
    else
    {
        bool bSelect = false;
        if (Current[0]->exec_time > Current[1]->exec_time)
        {
            // terminator
            bSelect = (gt > Current[1]->exec_time) && (gt < Current[0]->exec_time);
        }
        else
        {
            bSelect = (gt > Current[1]->exec_time);
        }
        if (bSelect)
        {
            Current[0] = Current[1];
            SelectEnv(CurrentWeather, Current[1], gt);
#ifdef WEATHER_LOGGING
            Msg("Weather: '%s' Desc: '%s' Time: %3.2f/%3.2f", CurrentWeatherName.c_str(),
                Current[1]->m_identifier.c_str(), Current[1]->exec_time, fGameTime);
#endif
        }
    }
}

void CEnvironment::lerp(float& current_weight)
{
    if (bWFX && (wfx_time <= 0.f))
        StopWFX();

    SelectEnvs(fGameTime);
    VERIFY(Current[0] && Current[1]);

    current_weight = TimeWeight(fGameTime, Current[0]->exec_time, Current[1]->exec_time);
    // modifiers
    CEnvModifier EM;
    EM.far_plane = 0;
    EM.fog_color.set(0, 0, 0);
    EM.fog_density = 0;
    EM.ambient.set(0, 0, 0);
    EM.sky_color.set(0, 0, 0);
    EM.hemi_color.set(0, 0, 0);
    EM.use_flags.zero();

    Fvector view = Device.vCameraPosition;
    float mpower = 0;
    for (auto& mit : Modifiers)
        mpower += EM.sum(mit, view);

    // final lerp
    CurrentEnv->lerp(this, *Current[0], *Current[1], current_weight, EM, mpower);
}

void CEnvironment::OnFrame()
{
#ifdef _EDITOR
    SetGameTime(fGameTime + Device.fTimeDelta * fTimeFactor, fTimeFactor);
    if (fsimilar(ed_to_time, DAY_LENGTH) && fsimilar(ed_from_time, 0.f))
    {
        if (fGameTime > DAY_LENGTH)
            fGameTime -= DAY_LENGTH;
    }
    else
    {
        if (fGameTime > ed_to_time)
        {
            fGameTime = fGameTime - ed_to_time + ed_from_time;
            Current[0] = Current[1] = 0;
        }
        if (fGameTime < ed_from_time)
        {
            fGameTime = ed_from_time;
            Current[0] = Current[1] = 0;
        }
    }
    if (!psDeviceFlags.is(rsEnvironment))
        return;
#else
    if (!g_pGameLevel)
        return;
#endif

    // if (pInput->iGetAsyncKeyState(SDL_SCANCODE_O)) SetWeatherFX("surge_day");
    float current_weight;
    lerp(current_weight);

    // Igor. Dynamic sun position.
    if (!GEnv.Render->is_sun_static() && useDynamicSunDir)
        calculate_dynamic_sun_dir();

#ifndef MASTER_GOLD
    if (CurrentEnv->sun_dir.y > 0)
    {
        Log("CurrentEnv->sun_dir", CurrentEnv->sun_dir);
        // Log("current_weight", current_weight);
        // Log("mpower", mpower);

        Log("Current[0]->sun_dir", Current[0]->sun_dir);
        Log("Current[1]->sun_dir", Current[1]->sun_dir);
    }
    VERIFY2(CurrentEnv->sun_dir.y < 0, "Invalid sun direction settings in lerp");
#endif // #ifndef MASTER_GOLD

    PerlinNoise1D->SetFrequency(wind_gust_factor * MAX_NOISE_FREQ);
    wind_strength_factor = clampr(PerlinNoise1D->GetContinious(Device.fTimeGlobal) + 0.5f, 0.f, 1.f);

    shared_str l_id = (current_weight < 0.5f) ? Current[0]->lens_flare_id : Current[1]->lens_flare_id;
    eff_LensFlare->OnFrame(l_id);
    shared_str t_id = (current_weight < 0.5f) ? Current[0]->tb_id : Current[1]->tb_id;
    eff_Thunderbolt->OnFrame(t_id, CurrentEnv->bolt_period, CurrentEnv->bolt_duration);
    eff_Rain->OnFrame();

    // ******************** Environment params (setting)
    m_pRender->OnFrame(*this);
}

void CEnvironment::calculate_dynamic_sun_dir()
{
    float g = (360.0f / 365.25f) * (180.0f + fGameTime / DAY_LENGTH);

    g = deg2rad(g);

    // Declination
    float D = 0.396372f - 22.91327f * _cos(g) + 4.02543f * _sin(g) - 0.387205f * _cos(2 * g) + 0.051967f * _sin(2 * g) -
        0.154527f * _cos(3 * g) + 0.084798f * _sin(3 * g);

    // Now calculate the time correction for solar angle:
    float TC =
        0.004297f + 0.107029f * _cos(g) - 1.837877f * _sin(g) - 0.837378f * _cos(2 * g) - 2.340475f * _sin(2 * g);

    // IN degrees
    float Longitude = -30.4f;

    float SHA = (fGameTime / (DAY_LENGTH / 24) - 12) * 15 + Longitude + TC;

    // Need this to correctly determine SHA sign
    if (SHA > 180)
        SHA -= 360;
    if (SHA < -180)
        SHA += 360;

    // IN degrees
    float const Latitude = 50.27f;
    float const LatitudeR = deg2rad(Latitude);

    // Now we can calculate the Sun Zenith Angle (SZA):
    float cosSZA = _sin(LatitudeR) * _sin(deg2rad(D)) + _cos(LatitudeR) * _cos(deg2rad(D)) * _cos(deg2rad(SHA));

    clamp(cosSZA, -1.0f, 1.0f);

    float SZA = acosf(cosSZA);
    float SEA = PI / 2 - SZA;

    // To finish we will calculate the Azimuth Angle (AZ):
    float cosAZ = 0.f;
    float const sin_SZA = _sin(SZA);
    float const cos_Latitude = _cos(LatitudeR);
    float const sin_SZA_X_cos_Latitude = sin_SZA * cos_Latitude;
    if (!fis_zero(sin_SZA_X_cos_Latitude))
        cosAZ = (_sin(deg2rad(D)) - _sin(LatitudeR) * _cos(SZA)) / sin_SZA_X_cos_Latitude;

    clamp(cosAZ, -1.0f, 1.0f);
    float AZ = acosf(cosAZ) + sunDirAzimuth;

    const Fvector2 minAngle = Fvector2().set(deg2rad(1.0f), deg2rad(3.0f));

    if (SEA < minAngle.x)
        SEA = minAngle.x;

    float fSunBlend = (SEA - minAngle.x) / (minAngle.y - minAngle.x);
    clamp(fSunBlend, 0.0f, 1.0f);

    SEA = -SEA;

    if (SHA < 0)
        AZ = 2 * PI - AZ;

    R_ASSERT(_valid(AZ));
    R_ASSERT(_valid(SEA));
    CurrentEnv->sun_dir.setHP(AZ, SEA);
    R_ASSERT(_valid(CurrentEnv->sun_dir));

    CurrentEnv->sun_color.mul(fSunBlend);
}

void CEnvironment::create_mixer()
{
    VERIFY(!CurrentEnv);
    CurrentEnv = new CEnvDescriptorMixer("00:00:00");
}

void CEnvironment::destroy_mixer() { xr_delete(CurrentEnv); }
SThunderboltDesc* CEnvironment::thunderbolt_description(CInifile& config, shared_str const& section)
{
    SThunderboltDesc* result = new SThunderboltDesc();
    result->load(config, section);
    return (result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection(CInifile* pIni, CInifile* thunderbolts, LPCSTR section)
{
    SThunderboltCollection* result = new SThunderboltCollection();
    result->load(pIni, thunderbolts, section);
    return (result);
}

SThunderboltCollection* CEnvironment::thunderbolt_collection(
    xr_vector<SThunderboltCollection*>& collection, shared_str const& id)
{
    for (auto& it : collection)
        if (it->section == id)
            return it;

    NODEFAULT;
    return nullptr;
}

CLensFlareDescriptor* CEnvironment::add_flare(xr_vector<CLensFlareDescriptor*>& collection, shared_str const& id)
{
    for (const auto& it: collection)
        if (it->section == id)
            return it;

    CLensFlareDescriptor* result = new CLensFlareDescriptor();
    result->load(m_suns_config, id.c_str());
    collection.push_back(result);
    return result;
}
