////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_thunderbolt.cpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts thunderbolt class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_thunderbolts_thunderbolt.hpp"
#include "ide.hpp"
#include "editor_environment_manager.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_detail.hpp"
#include "editor_environment_thunderbolts_manager.hpp"

using editor::environment::thunderbolts::thunderbolt;
using editor::environment::thunderbolts::manager;

thunderbolt::thunderbolt(manager* manager, shared_str const& id)
    : m_manager(*manager), m_id(id), m_property_holder(0), m_center(nullptr), m_top(nullptr)
{
}

thunderbolt::~thunderbolt()
{
    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void thunderbolt::create_top_gradient(const CInifile& config, shared_str const& section)
{
    VERIFY(section == m_id);
    m_top = xr_new<gradient>();
    m_top->load(config, section, "gradient_top");
    m_GradientTop = m_top;
}

void thunderbolt::create_center_gradient(const CInifile& config, shared_str const& section)
{
    VERIFY(section == m_id);
    m_center = xr_new<gradient>();
    m_center->load(config, section, "gradient_center");
    m_GradientCenter = m_center;
}

void thunderbolt::load(CInifile& config)
{
    inherited::load(config, m_id);
    m_color_animator = config.r_string(m_id, "color_anim");
    m_lighting_model = config.r_string(m_id, "lightning_model");
    m_sound = config.r_string(m_id, "sound");
}

void thunderbolt::save(CInifile& config)
{
    m_top->save(config, m_id, "gradient_top");
    m_center->save(config, m_id, "gradient_center");
    config.w_string(m_id.c_str(), "color_anim", m_color_animator.c_str());
    config.w_string(m_id.c_str(), "lightning_model", m_lighting_model.c_str());
    config.w_string(m_id.c_str(), "sound", m_sound.c_str());
}

pcstr thunderbolt::id_getter() const { return (m_id.c_str()); }
void thunderbolt::id_setter(pcstr value_)
{
    shared_str value = value_;
    if (m_id._get() == value._get())
        return;

    m_id = m_manager.unique_thunderbolt_id(value);
}

void thunderbolt::fill(::editor::environment::manager& environment, XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder(m_id.c_str(), collection, this);

    typedef XRay::Editor::property_holder_base::string_getter_type string_getter_type;
    string_getter_type string_getter;
    string_getter.bind(this, &thunderbolt::id_getter);

    typedef XRay::Editor::property_holder_base::string_setter_type string_setter_type;
    string_setter_type string_setter;
    string_setter.bind(this, &thunderbolt::id_setter);

    m_property_holder->add_property(
        "id", "properties", "this option is responsible for thunderbolt id", m_id.c_str(), string_getter, string_setter);

    m_property_holder->add_property("color animator", "properties",
        "this option is responsible for thunderbolt color animator", m_color_animator.c_str(), m_color_animator,
        &*environment.light_animator_ids().begin(), environment.light_animator_ids().size(),
        XRay::Editor::property_holder_base::value_editor_tree_view, XRay::Editor::property_holder_base::cannot_enter_text);

    m_property_holder->add_property("lighting model", "properties",
        "this option is responsible for thunderbolt lighting model", m_lighting_model.c_str(), m_lighting_model, ".dm",
        "Lighting model files (*.dm)|*.dm", detail::real_path("$game_meshes$", "").c_str(), "Select lighting model...",
        XRay::Editor::property_holder_base::cannot_enter_text, XRay::Editor::property_holder_base::keep_extension);

    m_property_holder->add_property("sound", "properties", "this option is responsible for thunderbolt sound",
        m_sound.c_str(), m_sound, ".ogg", "Sound files (*.ogg)|*.ogg", detail::real_path("$game_sounds$", "").c_str(),
        "Select sound...", XRay::Editor::property_holder_base::cannot_enter_text, XRay::Editor::property_holder_base::remove_extension);

    m_center->fill(
        environment, "center", "this option is responsible for thunderbolt gradient center", *m_property_holder);
    m_top->fill(environment, "top", "this option is responsible for thunderbolt gradient top", *m_property_holder);
}

thunderbolt::property_holder_type* thunderbolt::object() { return (m_property_holder); }
