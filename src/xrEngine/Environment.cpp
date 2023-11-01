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


//////////////////////////////////////////////////////////////////////////
// environment
CEnvironment::CEnvironment() : m_ambients_config(0)
{
    bWFX = false;
    Current[0] = 0;
    Current[1] = 0;
    CurrentWeather = 0;
    CurrentWeatherName = 0;
    eff_Rain = 0;
    eff_LensFlare = 0;
    eff_Thunderbolt = 0;
    OnDeviceCreate();

    fGameTime = 0.f;
    fTimeFactor = 12.f;

    wind_strength_factor = 0.f;
    wind_gust_factor = 0.f;

    wetness_factor = 0.f;

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
    PerlinNoise1D = xr_new<CPerlinNoise1D>(Random.randI(0, 0xFFFF));
    PerlinNoise1D->SetOctaves(2);
    PerlinNoise1D->SetAmplitude(0.66666f);

    // tsky0 = Device.Resources->_CreateTexture("$user$sky0");
    // tsky1 = Device.Resources->_CreateTexture("$user$sky1");

    string_path filePath;
    const auto load_config = [&filePath](pcstr path) -> CInifile*
    {
        if (FS.update_path(filePath, "$game_config$", path, false))
            return xr_new<CInifile>(filePath, true, true, false);
        return nullptr;
    };

    m_ambients_config                = load_config("environment\\ambients.ltx");
    m_sound_channels_config          = load_config("environment\\sound_channels.ltx");
    m_effects_config                 = load_config("environment\\effects.ltx");
}

CEnvironment::~CEnvironment()
{
    xr_delete(PerlinNoise1D);
    OnDeviceDestroy();

    CInifile::Destroy(m_ambients_config);
    m_ambients_config = nullptr;

    CInifile::Destroy(m_sound_channels_config);
    m_sound_channels_config = nullptr;

    CInifile::Destroy(m_effects_config);
    m_effects_config = nullptr;
}

void CEnvironment::Invalidate()
{
    bWFX = false;
    Current[0] = 0;
    Current[1] = 0;
    if (eff_LensFlare)
        eff_LensFlare->Invalidate();

    CurrentEnv.env_ambient = nullptr; // hack
    CurrentEnv.lens_flare  = nullptr; // hack
    CurrentEnv.thunderbolt = nullptr; // hack
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

void CEnvironment::ChangeGameTime(float game_time)
{
    fGameTime = NormalizeTime(fGameTime + game_time);
}

void CEnvironment::SetGameTime(float game_time, float time_factor)
{
    if (bWFX)
        wfx_time -= TimeDiff(fGameTime, game_time);
    fGameTime = game_time;
    fTimeFactor = time_factor;
}

void CEnvironment::SplitTime(float time, u32& hours, u32& minutes, u32& seconds) const
{
    u32 current_time_u32 = iFloor(time);
    current_time_u32 = current_time_u32 % (24 * 60 * 60);

    hours = current_time_u32 / (60 * 60);
    current_time_u32 %= (60 * 60);

    minutes = current_time_u32 / 60;
    seconds = current_time_u32 % 60;
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
    //. static bool bAlready = false;
    //. if(bAlready) return;
    if (name.size())
    {
        //. bAlready = true;
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
            CurrentEnv.soc_style = CurrentWeather->soc_style;
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
        CurrentEnv.soc_style = CurrentWeather->soc_style;

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
        for (auto t_it = CurrentWeather->begin() + 2; t_it != CurrentWeather->end() - 1; ++t_it)
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

    for (auto& env : *CurrentWeather)
        env->exec_time = NormalizeTime(env->exec_time - wfx_time + time);

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

void CEnvironment::lerp()
{
    if (bWFX && (wfx_time <= 0.f))
        StopWFX();

    SelectEnvs(fGameTime);
    VERIFY(Current[0] && Current[1]);

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
    const float current_weight = TimeWeight(fGameTime, Current[0]->exec_time, Current[1]->exec_time);
    CurrentEnv.lerp(*this, *Current[0], *Current[1], current_weight, EM, mpower);
    m_pRender->lerp(CurrentEnv, &*Current[0]->m_pDescriptor, &*Current[1]->m_pDescriptor);
}

void CEnvironment::OnFrame()
{
    if (!g_pGameLevel)
        return;

    lerp();

    PerlinNoise1D->SetFrequency(wind_gust_factor * MAX_NOISE_FREQ);
    wind_strength_factor = clampr(PerlinNoise1D->GetContinious(Device.fTimeGlobal) + 0.5f, 0.f, 1.f);

    eff_LensFlare->OnFrame(CurrentEnv, fTimeFactor);
    eff_Thunderbolt->OnFrame(CurrentEnv);
    eff_Rain->OnFrame();
}
