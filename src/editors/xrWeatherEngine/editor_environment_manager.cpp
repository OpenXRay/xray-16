////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_manager.cpp
// Created : 12.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment manager class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_manager.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_levels_manager.hpp"
#include "editor_environment_effects_manager.hpp"
#include "editor_environment_sound_channels_manager.hpp"
#include "editor_environment_ambients_manager.hpp"
#include "editor_environment_thunderbolts_manager.hpp"
#include "editor_environment_weathers_manager.hpp"
#include "editor_environment_detail.hpp"
#include "ide.hpp"
#include "Common/object_broker.h"
#include "xrEngine/LightAnimLibrary.h"
#include "editor_environment_weathers_time.hpp"
#include "Include/xrRender/particles_systems_library_interface.hpp"
#include "editor_environment_ambients_ambient.hpp"
#include "xrEngine/xr_efflensflare.h"

namespace editor::environment
{
manager::manager()
    : m_suns(0), m_levels(0), m_effects(0), m_sound_channels(0), m_ambients(0), m_thunderbolts(0), m_weathers(0)
{
    m_effects = xr_new<editor::environment::effects::manager>(this);
    m_sound_channels = xr_new<editor::environment::sound_channels::manager>();
    m_ambients = xr_new<editor::environment::ambients::manager>(*this);
    m_weathers = xr_new<editor::environment::weathers::manager>(this);
    m_suns = xr_new<editor::environment::suns::manager>(this);
    m_levels = xr_new<editor::environment::levels::manager>(m_weathers);
    m_thunderbolts = xr_new<editor::environment::thunderbolts::manager>(this);

    load_internal();
    fill();
}

manager::~manager()
{
    xr_delete(m_thunderbolts);
    xr_delete(m_ambients);
    xr_delete(m_sound_channels);
    xr_delete(m_effects);
    xr_delete(m_levels);
    xr_delete(m_weathers);
    xr_delete(m_suns);

    delete_data(m_shader_ids);
    delete_data(m_light_animator_ids);

    WeatherCycles.clear();
    WeatherFXs.clear();

    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void manager::load() {}
void manager::load_internal()
{
    m_thunderbolts->load();
    m_suns->load();
    m_levels->load();
    m_effects->load();
    m_sound_channels->load();
    m_ambients->load();

    inherited::load();
}

void manager::save()
{
    m_weathers->save();
    //m_suns->save(); // На текущий момент сохраняются/загружаются не все параметры.
    m_ambients->save();
    m_effects->save();
    m_sound_channels->save();
    m_thunderbolts->save();
    //m_levels->save(); // На текущий момент сохранение происходит во время выхода из редактора.
}

void manager::fill()
{
    m_property_holder = ::ide().create_property_holder("environment");

    m_weathers->fill(m_property_holder);
    m_suns->fill(m_property_holder);
    m_ambients->fill(m_property_holder);
    m_effects->fill(m_property_holder);
    m_sound_channels->fill(m_property_holder);
    m_thunderbolts->fill(m_property_holder);
    m_levels->fill();

    ::ide().environment_weathers(m_property_holder);
}

void manager::load_weathers()
{
    m_weathers->load();

    for (auto& i : WeatherCycles)
    {
        R_ASSERT3(i.second.size() > 1, "Environment in weather must >=2", *i.first);
        std::sort(i.second.begin(), i.second.end(), sort_env_etl_pred);
    }
    R_ASSERT2(!WeatherCycles.empty(), "Empty weathers.");
    SetWeather((*WeatherCycles.begin()).first.c_str());
}

manager::shader_ids_type const& manager::shader_ids() const
{
    if (!m_shader_ids.empty())
        return (m_shader_ids);

    string_path path;
    FS.update_path(path, "$game_data$", "shaders.xr");
    IReader* reader = FS.r_open(path);
    IReader* stream = reader->open_chunk(3);
    R_ASSERT(stream);

    u32 count = stream->r_u32();
    m_shader_ids.resize(count);

    for (auto& i : m_shader_ids)
    {
        string_path buffer;
        stream->r_stringZ(buffer, sizeof(buffer));
        i = xr_strdup(buffer);
    }

    stream->close();
    FS.r_close(reader);

    std::sort(m_shader_ids.begin(), m_shader_ids.end(), detail::logical_string_predicate());

    return (m_shader_ids);
}

manager::particle_ids_type const& manager::particle_ids() const
{
    if (!m_particle_ids.empty())
        return (m_particle_ids);

    auto const& library = m_pRender->particles_systems_library();
    PS::CPGDef const* const* i = library.particles_group_begin();
    PS::CPGDef const* const* e = library.particles_group_end();
    for (; i != e; library.particles_group_next(i))
        m_particle_ids.push_back(library.particles_group_id(**i).c_str());

    std::sort(m_particle_ids.begin(), m_particle_ids.end(), detail::logical_string_predicate());
    return (m_particle_ids);
}

manager::light_animator_ids_type const& manager::light_animator_ids() const
{
    if (!m_light_animator_ids.empty())
        return (m_light_animator_ids);

    typedef LAItemVec container_type;
    container_type const& light_animators = LALib.Objects();
    m_light_animator_ids.resize(light_animators.size());

    auto j = m_light_animator_ids.begin();
    for (const auto& i : light_animators)
        *j++ = xr_strdup(i->cName.c_str());

    std::sort(m_light_animator_ids.begin(), m_light_animator_ids.end(), detail::logical_string_predicate());

    return (m_light_animator_ids);
}

void manager::create_mixer()
{
    VERIFY(!CurrentEnv);
    editor::environment::weathers::time* object = xr_new<editor::environment::weathers::time>(this, (editor::environment::weathers::weather const*)0, "");
    CurrentEnv = object;
    object->fill(0);
}

void manager::unload()
{
    WeatherCycles.clear();
    WeatherFXs.clear();
    Modifiers.clear();
    Ambients.clear();
}

CEnvAmbient* manager::AppendEnvAmb(const shared_str& sect, CInifile const* /*pIni = nullptr*/) { return (m_ambients->get_ambient(sect)); }
SThunderboltDesc* manager::thunderbolt_description(const CInifile& config, shared_str const& section)
{
    return (m_thunderbolts->description(config, section));
}

SThunderboltCollection* manager::thunderbolt_collection(CInifile const* pIni, CInifile const* thunderbolts, pcstr section)
{
    return (m_thunderbolts->get_collection(section));
}

SThunderboltCollection* manager::thunderbolt_collection(
    xr_vector<SThunderboltCollection*>& collection, shared_str const& id)
{
    return (m_thunderbolts->get_collection(id));
}
} // namespace editor::environment
