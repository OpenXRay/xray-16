////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_effects_effect.cpp
// Created : 28.12.2007
// Modified : 28.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment effects effect class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "editor_environment_effects_effect.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "editor_environment_manager.hpp"
#include "ide.hpp"
#include "editor_environment_effects_manager.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::effects::effect;
using editor::environment::effects::manager;

effect::effect(manager const& manager, shared_str const& id)
    : m_manager(manager), m_property_holder(0), m_id(id), m_sound("")
{
    particles = "";
}

effect::~effect()
{
    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void effect::load(const CInifile& config)
{
    life_time = config.r_u32(m_id, "life_time");
    offset = config.r_fvector3(m_id, "offset");
    particles = config.r_string(m_id, "particles");
    m_sound = config.r_string(m_id, "sound");
    wind_gust_factor = config.r_float(m_id, "wind_gust_factor");
    wind_blast_in_time = config.r_float(m_id, "wind_blast_in_time");
    wind_blast_out_time = config.r_float(m_id, "wind_blast_out_time");
    wind_blast_strength = config.r_float(m_id, "wind_blast_strength");
    wind_blast_direction.setHP(deg2rad(config.r_float(m_id, "wind_blast_longitude")), 0.f);
}

void effect::save(CInifile& config)
{
    config.w_u32(m_id.c_str(), "life_time", life_time);
    config.w_fvector3(m_id.c_str(), "offset", offset);
    config.w_string(m_id.c_str(), "particles", particles.c_str());
    config.w_string(m_id.c_str(), "sound", m_sound.c_str());
    config.w_float(m_id.c_str(), "wind_gust_factor", wind_gust_factor);
    config.w_float(m_id.c_str(), "wind_blast_in_time", wind_blast_in_time);
    config.w_float(m_id.c_str(), "wind_blast_out_time", wind_blast_out_time);
    config.w_float(m_id.c_str(), "wind_blast_strength", wind_blast_strength);
    config.w_float(m_id.c_str(), "wind_blast_longitude", rad2deg(wind_blast_direction.getH()));
}

pcstr effect::id_getter() const { return (m_id.c_str()); }
void effect::id_setter(pcstr value_)
{
    shared_str value = value_;
    if (m_id._get() == value._get())
        return;

    m_id = m_manager.unique_id(value);
}

pcstr effect::sound_getter() { return (m_sound.c_str()); }
void effect::sound_setter(pcstr value)
{
    m_sound = value;
    sound.destroy();
    sound.create(value, st_Effect, sg_SourceType);
}

float effect::wind_blast_longitude_getter() const
{
    float h, p;
    wind_blast_direction.getHP(h, p);
    return (rad2deg(h));
}

void effect::wind_blast_longitude_setter(float value) { wind_blast_direction.setHP(deg2rad(value), 0.f); }
void effect::fill(XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder(m_id.c_str(), collection, this);

    typedef XRay::Editor::property_holder_base::string_getter_type string_getter_type;
    string_getter_type string_getter;
    string_getter.bind(this, &effect::id_getter);

    typedef XRay::Editor::property_holder_base::string_setter_type string_setter_type;
    string_setter_type string_setter;
    string_setter.bind(this, &effect::id_setter);

    m_property_holder->add_property("id", "properties", "this option is responsible for effect identifier", m_id.c_str(),
        string_getter, string_setter);

    m_property_holder->add_property("life time", "properties",
        "this option is responsible for effect life time (in milliseconds)", (int const&)life_time, (int&)life_time);

    m_property_holder->add_property("offset", "properties", "this option is responsible for effect offset (3D vector)",
        (XRay::Editor::vec3f const&)offset, (XRay::Editor::vec3f&)offset);

    m_property_holder->add_property("particles", "properties", "this option is responsible for effect particles",
        particles.c_str(), particles, &*m_manager.environment().particle_ids().begin(),
        m_manager.environment().particle_ids().size(), XRay::Editor::property_holder_base::value_editor_tree_view,
                                    XRay::Editor::property_holder_base::cannot_enter_text);

    string_getter.bind(this, &effect::sound_getter);
    string_setter.bind(this, &effect::sound_setter);
    m_property_holder->add_property("sound", "properties", "this option is responsible for effect sound",
        m_sound.c_str(), string_getter, string_setter, ".ogg", "Sound files (*.ogg)|*.ogg",
        detail::real_path("$game_sounds$", "").c_str(), "Select sound...", XRay::Editor::property_holder_base::cannot_enter_text,
        XRay::Editor::property_holder_base::remove_extension);

    m_property_holder->add_property("wind gust factor", "properties",
        "this option is responsible for effect wind gust factor", wind_gust_factor, wind_gust_factor);

    m_property_holder->add_property("wind blast strength", "properties",
        "this option is responsible for effect wind blast strength", wind_blast_strength, wind_blast_strength);

    m_property_holder->add_property("wind blast start time", "properties",
        "this option is responsible for effect wind blast start time", wind_blast_in_time, wind_blast_in_time, 0.f,
        1000.f);

    m_property_holder->add_property("wind blast stop time", "properties",
        "this option is responsible for effect wind blast stop time", wind_blast_out_time, wind_blast_out_time, 0.f,
        1000.f);

    typedef XRay::Editor::property_holder_base::float_getter_type float_getter_type;
    float_getter_type float_getter;

    typedef XRay::Editor::property_holder_base::float_setter_type float_setter_type;
    float_setter_type float_setter;

    float_getter.bind(this, &effect::wind_blast_longitude_getter);
    float_setter.bind(this, &effect::wind_blast_longitude_setter);

    m_property_holder->add_property("wind blast longitude", "properties",
        "this option is responsible for effect wind blast longitude", float_getter(), float_getter, float_setter, -360.f,
        360.f);
}

XRay::Editor::property_holder_base* effect::object() { return m_property_holder; }
