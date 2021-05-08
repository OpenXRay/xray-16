////////////////////////////////////////////////////////////////////////////
// Module : engine_impl.cpp
// Created : 04.12.2007
// Modified : 04.12.2007
// Author : Dmitriy Iassenev
// Description : engine implementation class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "engine_impl.hpp"
#include "xrEngine/XR_IOConsole.h"
#include "xrEngine/xr_input.h"
#include "xrEngine/IGame_Persistent.h"
#include "xrEngine/IGame_Level.h"
#include "editor_environment_weathers_time.hpp"
#include "editor_environment_manager.hpp"
#include "editor_environment_weathers_manager.hpp"

ENGINE_API extern CConsole* Console;
engine_impl g_engine;

engine_impl::engine_impl() : m_input_receiver(xr_new<IInputReceiver>()), m_input_captured(false) {}
engine_impl::~engine_impl()
{
    capture_input(false);
    xr_delete(m_input_receiver);
}
#if !defined(LINUX)
bool engine_impl::on_message(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    return (Device.on_message(hWnd, uMsg, wParam, lParam, result));
}
#endif
void engine_impl::on_idle()
{
    SDL_PumpEvents();
    Device.ProcessFrame();
}
void engine_impl::on_resize()
{
    if (Console)
        Console->Execute("vid_restart");
}

void engine_impl::pause(bool const& value)
{
    if (value == !!Device.Paused())
        return;

    Device.Pause(value ? TRUE : FALSE, TRUE, TRUE, "editor query");
}

void engine_impl::capture_input(bool const& value)
{
    if (value == m_input_captured)
        return;

    m_input_captured = value;

    if (value)
        m_input_receiver->IR_Capture();
    else
        m_input_receiver->IR_Release();
}

void engine_impl::disconnect() { Console->Execute("quit"); }

bool engine_impl::quit_requested() const
{
    return SDL_QuitRequested();
}

void engine_impl::value(LPCSTR value, shared_str& result) { result = value; }
LPCSTR engine_impl::value(shared_str const& value) { return (value.c_str()); }

CEnvironment* engine_impl::environment()
{
    return xr_new<editor::environment::manager>();
}

void engine_impl::weather(LPCSTR value)
{
    if (!g_pGamePersistent)
        return;

    shared_str new_weather_id = value;
    CEnvironment& environment = g_pGamePersistent->Environment();
    if (environment.CurrentWeatherName._get() == new_weather_id._get())
        return;

    typedef CEnvironment::EnvsMap EnvsMap;
    EnvsMap const& weathers = environment.WeatherCycles;
    EnvsMap::const_iterator i = weathers.find(value);
    if (i == weathers.end())
        return;

    float const game_time = g_pGamePersistent->Environment().GetGameTime();
    environment.SetWeather(value, true);
    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(game_time), environment.fTimeFactor);
    g_pGamePersistent->Environment().SelectEnvs(game_time);
}

LPCSTR engine_impl::weather()
{
    if (!g_pGamePersistent)
        return ("");

    return (g_pGamePersistent->Environment().GetWeather().c_str());
}

void engine_impl::current_weather_frame(LPCSTR frame_id)
{
    if (!g_pGamePersistent)
        return;

    shared_str new_frame_id = frame_id;

    CEnvironment& environment = g_pGamePersistent->Environment();
    VERIFY(environment.CurrentWeather);
    typedef CEnvironment::EnvVec EnvVec;
    EnvVec const& frames = *environment.CurrentWeather;
    EnvVec::const_iterator i = frames.begin();
    EnvVec::const_iterator e = frames.end();
    for (; i != e; ++i)
        if ((*i)->m_identifier._get() == new_frame_id._get())
        {
            environment.Current[0] = (*i);
            environment.Current[1] = ((i + 1) == e) ? (*frames.begin()) : *(i + 1);
            bool set_time = true;
            if (environment.Current[0]->exec_time < environment.Current[1]->exec_time)
            {
                if (environment.GetGameTime() > environment.Current[0]->exec_time)
                    if (environment.GetGameTime() < environment.Current[1]->exec_time)
                        set_time = false;
            }
            else
            {
                if (environment.GetGameTime() > environment.Current[0]->exec_time)
                    set_time = false;
            }
            if (set_time)
            {
                environment.SetGameTime((*i)->exec_time, environment.fTimeFactor);
                g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor((*i)->exec_time * 1000.f), environment.fTimeFactor);
            }
            break;
        }
}

LPCSTR engine_impl::current_weather_frame()
{
    if (!g_pGamePersistent)
        return ("");

    if (!g_pGamePersistent->Environment().Current[0])
        return ("");

    return (g_pGamePersistent->Environment().Current[0]->m_identifier.c_str());
}

void engine_impl::track_frame(float const& time)
{
    VERIFY(time >= 0.f);
    VERIFY(time <= 1.f);

    if (!g_pGameLevel)
        return;

    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!environment.Current[0])
        return;

    if (!environment.Current[1])
        return;

    float start_time = environment.Current[0]->exec_time;
    float stop_time = environment.Current[1]->exec_time;
    float current_time;
    if (start_time < stop_time)
        current_time = start_time + time * (stop_time - start_time);
    else
    {
        stop_time += 24.f * 60.f * 60.f;
        current_time = start_time + time * (stop_time - start_time);
        if (current_time >= 24.f * 60.f * 60.f)
            current_time -= 24.f * 60.f * 60.f;
    }
    environment.SetGameTime(current_time, environment.fTimeFactor);
    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(current_time * 1000.f), environment.fTimeFactor);
}

float engine_impl::track_frame()
{
    if (!g_pGamePersistent)
        return (0.f);

    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!environment.Current[0])
        return (0.f);

    float start_time = environment.Current[0]->exec_time;
    float stop_time = environment.Current[1]->exec_time;
    float current_time = g_pGamePersistent->Environment().GetGameTime();
    if (start_time >= stop_time)
    {
        if (current_time >= start_time)
            clamp(current_time, start_time, 24.f * 60.f * 60.f);
        else
            clamp(current_time, 0.f, stop_time);

        if (current_time <= stop_time)
            current_time += 24.f * 60.f * 60.f;

        stop_time += 24.f * 60.f * 60.f;
    }
    else
        clamp(current_time, start_time, stop_time);

    VERIFY(start_time < stop_time);
    return ((current_time - start_time) / (stop_time - start_time));
}

void engine_impl::track_weather(float const& time)
{
    VERIFY(time < 24 * 60 * 60);

    auto& environment = g_pGamePersistent->Environment();

    const bool paused = environment.m_paused;

    environment.m_paused = false;
    environment.SetGameTime(time * 24 * 60 * 60, environment.fTimeFactor);
    environment.m_paused = true;
    environment.SetGameTime(time * 24 * 60 * 60, environment.fTimeFactor);

    environment.m_paused = paused;

    float weight;
    environment.Invalidate();
    environment.lerp(weight);
}

float engine_impl::track_weather() { return (g_pGamePersistent->Environment().GetGameTime() / (24 * 60 * 60)); }

XRay::Editor::property_holder_base* engine_impl::current_frame_property_holder()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!environment.Current[0])
        return (0);

    return (((editor::environment::weathers::time&)(*environment.Current[0])).object());
}

XRay::Editor::property_holder_base* engine_impl::blend_frame_property_holder()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!environment.CurrentEnv)
        return (0);

    return (((editor::environment::weathers::time&)(*environment.CurrentEnv)).object());
}

XRay::Editor::property_holder_base* engine_impl::target_frame_property_holder()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    if (!environment.Current[1])
        return (0);

    return (((editor::environment::weathers::time&)(*environment.Current[1])).object());
}

void engine_impl::weather_paused(bool const& value) { g_pGamePersistent->Environment().m_paused = value; }
bool engine_impl::weather_paused() { return (g_pGamePersistent->Environment().m_paused); }
void engine_impl::weather_time_factor(float const& value_raw)
{
    float value = value_raw;
    clamp(value, .01f, 100000.f);

    if (g_pGameLevel && g_pGamePersistent)
        g_pGameLevel->SetEnvironmentGameTimeFactor(
            iFloor(g_pGamePersistent->Environment().GetGameTime() * 1000.f), value);

    if (g_pGamePersistent)
        g_pGamePersistent->Environment().fTimeFactor = value;
}

float engine_impl::weather_time_factor()
{
    if (!g_pGamePersistent)
        return (1.f);

    return (g_pGamePersistent->Environment().fTimeFactor);
}

void engine_impl::save_weathers()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    manager.save();
}

bool engine_impl::save_time_frame(char* buffer, u32 const& buffer_size)
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    return (manager.weathers().save_current_blend(buffer, buffer_size));
}

bool engine_impl::paste_current_time_frame(char const* buffer, u32 const& buffer_size)
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    return (manager.weathers().paste_current_time_frame(buffer, buffer_size));
}

bool engine_impl::paste_target_time_frame(char const* buffer, u32 const& buffer_size)
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    return (manager.weathers().paste_target_time_frame(buffer, buffer_size));
}

bool engine_impl::add_time_frame(char const* buffer, u32 const& buffer_size)
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    return (manager.weathers().add_time_frame(buffer, buffer_size));
}

char const* engine_impl::weather_current_time() const
{
    return (g_pGamePersistent->Environment().CurrentEnv->m_identifier.c_str());
}

void engine_impl::weather_current_time(char const* time)
{
    u32 hours, minutes, seconds;
    sscanf(time, "%u:%u:%u", &hours, &minutes, &seconds);
    
    auto& environment = g_pGamePersistent->Environment();

    const bool paused = environment.m_paused;

    environment.m_paused = false;
    environment.SetGameTime(
        float(hours * 60 * 60 + minutes * 60 + seconds), environment.fTimeFactor);
    environment.m_paused = paused;

    float weight;
    environment.Invalidate();
    environment.lerp(weight);
}

void engine_impl::reload_current_time_frame()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    manager.weathers().reload_current_time_frame();
}

void engine_impl::reload_target_time_frame()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);
    manager.weathers().reload_target_time_frame();
}

void engine_impl::reload_current_weather()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);

    float const game_time = environment.GetGameTime();
    manager.weathers().reload_current_weather();
    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(game_time), environment.fTimeFactor);
    environment.Current[0] = 0;
    environment.Current[1] = 0;
    environment.SelectEnvs(game_time);
    VERIFY(environment.Current[1]);
    if (environment.Current[1]->exec_time == game_time)
        environment.SelectEnvs(game_time + .1f);
}

void engine_impl::reload_weathers()
{
    CEnvironment& environment = g_pGamePersistent->Environment();
    editor::environment::manager& manager = dynamic_cast<editor::environment::manager&>(environment);

    float const game_time = environment.GetGameTime();
    manager.weathers().reload();
    g_pGameLevel->SetEnvironmentGameTimeFactor(iFloor(game_time), environment.fTimeFactor);
    environment.Current[0] = 0;
    environment.Current[1] = 0;
    environment.SelectEnvs(game_time);
    VERIFY(environment.Current[1]);
    if (environment.Current[1]->exec_time == game_time)
        environment.SelectEnvs(game_time + .1f);
}

