////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_manager.cpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "editor_environment_thunderbolts_manager.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_thunderbolts_thunderbolt.hpp"
#include "editor_environment_thunderbolts_thunderbolt_id.hpp"
#include "editor_environment_thunderbolts_collection.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_manager.hpp"
#include "IGame_Persistent.h"

using editor::environment::thunderbolts::manager;
using editor::environment::thunderbolts::thunderbolt;
using editor::environment::thunderbolts::thunderbolt_id;
using editor::environment::thunderbolts::collection;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::thunderbolt_container_type, manager>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<manager::thunderbolt_container_type, manager>::create()
{
    thunderbolt* object = new thunderbolt(&m_holder, generate_unique_id("thunderbolt_unique_id_").c_str());
    object->fill(m_holder.environment(), this);
    return (object->object());
}

template <>
void property_collection<manager::collection_container_type, manager>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<manager::collection_container_type, manager>::create()
{
    collection* object = new collection(m_holder, generate_unique_id("thunderbolt_collection_unique_id_").c_str());
    object->fill(this);
    return (object->object());
}

manager::manager(::editor::environment::manager* environment)
    : m_thunderbolt_collection(0), m_thunderbolts_changed(true), m_collections_collection(0),
      m_collections_changed(true), m_property_holder(0), m_environment(*environment)
{
    m_thunderbolt_collection = new thunderbolt_collection_type(&m_thunderbolts, this, &m_thunderbolts_changed);
    m_collections_collection = new collection_collection_type(&m_collections, this, &m_collections_changed);
}

manager::~manager()
{
    xr_delete(m_thunderbolt_collection);
    delete_data(m_thunderbolts);

    xr_delete(m_collections_collection);
    delete_data(m_collections);

    delete_data(m_thunderbolts_ids);
    delete_data(m_collections_ids);

    if (!Device.editor() || !m_property_holder)
        return;

    ::ide().destroy(m_property_holder);
}

void manager::load_thunderbolts()
{
    VERIFY(m_thunderbolts.empty());

    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolts.ltx"), true, true, false);

    typedef CInifile::Root sections_type;
    sections_type& sections = config->sections();
    m_thunderbolts.reserve(sections.size());

    for (const auto &i : sections)
    {
        thunderbolt* object = new thunderbolt(this, i->Name);
        object->load(*config);
        object->fill(m_environment, m_thunderbolt_collection);
        m_thunderbolts.push_back(object);
    }

    xr_delete(config);
}

void manager::save_thunderbolts()
{
    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolts.ltx"), false, false, true);

    for (const auto &i : m_thunderbolts)
        i->save(*config);

    xr_delete(config);
}

void manager::load_collections()
{
    VERIFY(m_collections.empty());

    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolt_collections.ltx"), true, true, false);

    typedef CInifile::Root sections_type;
    sections_type& sections = config->sections();
    m_collections.reserve(sections.size());

    for (const auto &i : sections)
    {
        collection* object = new collection(*this, i->Name);
        object->load(*config);
        object->fill(m_thunderbolt_collection);
        m_collections.push_back(object);
    }

    xr_delete(config);
}

void manager::save_collections()
{
    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "thunderbolt_collections.ltx"), false, false, true);

    for (const auto &i : m_collections)
        i->save(*config);

    xr_delete(config);
}

void manager::load()
{
    load_thunderbolts();
    load_collections();
}

void manager::save()
{
    save_thunderbolts();
    save_collections();

    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "environment.ltx"), false, false, true);

    CEnvironment& environment = g_pGamePersistent->Environment();

    config->w_float("environment", "altitude", rad2deg(environment.p_var_alt));
    config->w_float("environment", "delta_longitude", rad2deg(environment.p_var_long));
    config->w_float("environment", "min_dist_factor", environment.p_min_dist);
    config->w_float("environment", "tilt", rad2deg(environment.p_tilt));
    config->w_float("environment", "second_propability", environment.p_second_prop);
    config->w_float("environment", "sky_color", environment.p_sky_color);
    config->w_float("environment", "sun_color", environment.p_sun_color);
    config->w_float("environment", "fog_color", environment.p_fog_color);

    xr_delete(config);
}

float manager::altitude_getter() const { return (rad2deg(m_environment.p_var_alt)); }
void manager::altitude_setter(float value) { m_environment.p_var_alt = deg2rad(value); }
float manager::longitude_getter() const { return (rad2deg(m_environment.p_var_long)); }
void manager::longitude_setter(float value) { m_environment.p_var_long = deg2rad(value); }
float manager::tilt_getter() const { return (rad2deg(m_environment.p_tilt)); }
void manager::tilt_setter(float value) { m_environment.p_tilt = deg2rad(value); }
void manager::fill(XRay::Editor::property_holder_base* holder)
{
    VERIFY(holder);

    typedef XRay::Editor::property_holder_base::float_getter_type float_getter_type;
    float_getter_type float_getter;

    typedef XRay::Editor::property_holder_base::float_setter_type float_setter_type;
    float_setter_type float_setter;

    float_getter.bind(this, &manager::altitude_getter);
    float_setter.bind(this, &manager::altitude_setter);

    holder->add_property("altitude", "thunderbolts", "this option is responsible for thunderbolts altitude (in degrees)",
        rad2deg(m_environment.p_var_alt), float_getter, float_setter, -360.0f, 360.f);

    float_getter.bind(this, &manager::longitude_getter);
    float_setter.bind(this, &manager::longitude_setter);

    holder->add_property("delta longitude", "thunderbolts",
        "this option is responsible for thunderbolts delta longitude (in degrees)", m_environment.p_var_long,
        float_getter, float_setter, -360.0f, 360.f);

    holder->add_property("minimum distance factor", "thunderbolts",
        "this option is responsible for thunderbolts minimum distance factor (distance from far plane)",
        m_environment.p_min_dist, m_environment.p_min_dist, .0f, .95f);

    float_getter.bind(this, &manager::tilt_getter);
    float_setter.bind(this, &manager::tilt_setter);

    holder->add_property("tilt", "thunderbolts", "this option is responsible for thunderbolts tilt (in degrees)",
        m_environment.p_tilt, float_getter, float_setter, 15.f, 30.f);

    holder->add_property("second probability", "thunderbolts",
        "this option is responsible for thunderbolts second probability (0..1)", m_environment.p_second_prop,
        m_environment.p_second_prop, 0.f, 1.f);

    holder->add_property("sky color", "thunderbolts", "this option is responsible for thunderbolts sky color (factor)",
        m_environment.p_sky_color, m_environment.p_sky_color, 0.f, 1.f);

    holder->add_property("sun color", "thunderbolts", "this option is responsible for thunderbolts sun color (factor)",
        m_environment.p_sun_color, m_environment.p_sun_color, 0.f, 1.f);

    holder->add_property("fog color", "thunderbolts", "this option is responsible for thunderbolts fog color (factor)",
        m_environment.p_fog_color, m_environment.p_fog_color, 0.f, 1.f);

    holder->add_property("thunderbolt collections", "thunderbolts",
        "this option is responsible for thunderbolt collections", m_collections_collection);

    holder->add_property(
        "thunderbolts", "thunderbolts", "this option is responsible for thunderbolts", m_thunderbolt_collection);
}

manager::thunderbolts_ids_type const& manager::thunderbolts_ids() const
{
    if (!m_thunderbolts_changed)
        return (m_thunderbolts_ids);

    delete_data(m_thunderbolts_ids);

    m_thunderbolts_ids.resize(m_thunderbolts.size());

    auto j = m_thunderbolts_ids.begin();
    for (const auto &i : m_thunderbolts)
        *j++ = xr_strdup(i->id());

    std::sort(m_thunderbolts_ids.begin(), m_thunderbolts_ids.end(), logical_string_predicate());

    return (m_thunderbolts_ids);
}

manager::thunderbolts_ids_type const& manager::collections_ids() const
{
    if (!m_collections_changed)
        return (m_collections_ids);

    delete_data(m_collections_ids);

    m_collections_ids.resize(m_collections.size() + 1);
    m_collections_ids[0] = xr_strdup("");

    auto j = m_collections_ids.begin() + 1;
    for (const auto &i : m_collections)
        *j++ = xr_strdup(i->id());

    std::sort(m_collections_ids.begin(), m_collections_ids.end(), logical_string_predicate());

    return (m_collections_ids);
}

::editor::environment::manager& manager::environment() const { return (m_environment); }
shared_str manager::unique_thunderbolt_id(shared_str const& id) const
{
    if (m_thunderbolt_collection->unique_id(id.c_str()))
        return (id);

    return (m_thunderbolt_collection->generate_unique_id(id.c_str()));
}

shared_str manager::unique_collection_id(shared_str const& id) const
{
    if (m_collections_collection->unique_id(id.c_str()))
        return (id);

    return (m_collections_collection->generate_unique_id(id.c_str()));
}

SThunderboltDesc* manager::description(CInifile& config, shared_str const& section) const
{
    for (const auto &i : m_thunderbolts)
        if (i->id() == section)
            return i;

    NODEFAULT;
#ifdef DEBUG
    return (0);
#endif // #ifdef DEBUG
}

SThunderboltCollection* manager::get_collection(shared_str const& section)
{
    for (const auto &i : m_collections)
        if (i->id() == section)
            return i;

    NODEFAULT;
#ifdef DEBUG
    return (0);
#endif // #ifdef DEBUG
}

