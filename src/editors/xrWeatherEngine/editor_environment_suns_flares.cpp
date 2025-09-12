////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_flares.cpp
// Created : 26.01.2008
// Modified : 26.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns flares class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_suns_flares.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection.hpp"
#include "editor_environment_suns_flare.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_manager.hpp"

using suns_flares = editor::environment::suns::flares;

template <>
void property_collection<suns_flares::flares_type, suns_flares>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    const float position = m_container[item_index]->m_position;
    xr_sprintf(buffer, buffer_size, "flare [%f]", position);
}

template <>
XRay::Editor::property_holder_base* property_collection<
    suns_flares::flares_type, suns_flares>::create()
{
    using editor::environment::suns::flare;

    auto* object = xr_new<flare>();
    object->fill(this);
    return (object->object());
}

namespace editor::environment::suns
{
flares::flares() : m_shader(""), m_collection(nullptr), m_use(false)
{
    m_collection = xr_new<collection_type>(&m_flares, this);
}

flares::~flares()
{
    xr_delete(m_collection);
    delete_data(m_flares);
}

void flares::load(CInifile& config, shared_str const& section)
{
    m_use = config.read_if_exists<bool>(section, "flares", true);
    m_shader = READ_IF_EXISTS(&config, r_string, section, "flare_shader", "effects\\flare");

    if (!m_use)
        return;

    const shared_str flare_opacity =
        READ_IF_EXISTS(&config, r_string, section, "flare_opacity", "0.340, 0.260, 0.500, 0.420, 0.260, 0.260");
    const shared_str flare_position =
        READ_IF_EXISTS(&config, r_string, section, "flare_position", "1.300, 1.000, 0.500, -0.300, -0.600, -1.000");
    const shared_str flare_radius =
        READ_IF_EXISTS(&config, r_string, section, "flare_radius", "0.080, 0.120, 0.040, 0.080, 0.120, 0.300");
    const shared_str flare_textures =
        READ_IF_EXISTS(&config, r_string, section, "flare_textures",
        "fx\\fx_flare1.tga, fx\\fx_flare2.tga, fx\\fx_flare2.tga, fx\\fx_flare2.tga, fx\\fx_flare3.tga, fx\\fx_flare1.tga");

    const u32 opacity_count = _GetItemCount(flare_opacity.c_str());
    const u32 position_count = _GetItemCount(flare_position.c_str());
    const u32 radius_count = _GetItemCount(flare_radius.c_str());
    const u32 texture_count = _GetItemCount(flare_textures.c_str());

    const u32 min_flare_count = _min(_min(_min(opacity_count, position_count), radius_count), texture_count);
    const u32 max_flare_count = _max(_max(_max(opacity_count, position_count), radius_count), texture_count);

    const u32 max_string_count =
        _max(_max(_max(flare_opacity.size(), flare_position.size()), flare_radius.size()), flare_textures.size()) + 1;

    if (min_flare_count != max_flare_count)
        Msg("! flare count for sun [%s] is setup incorrectly. only %d flares are correct", section.c_str(),
            min_flare_count);

    u32 const buffer_size = max_string_count * sizeof(char);
    pstr result = (pstr)xr_alloca(buffer_size);
    for (u32 i = 0; i < min_flare_count; ++i)
    {
        auto object = m_flares.emplace_back(xr_new<flare>());
        object->m_opacity = (float)atof(_GetItem(flare_opacity.c_str(), i, result, buffer_size));
        object->m_position = (float)atof(_GetItem(flare_position.c_str(), i, result, buffer_size));
        object->m_radius = (float)atof(_GetItem(flare_radius.c_str(), i, result, buffer_size));
        object->m_texture = _GetItem(flare_textures.c_str(), i, result, buffer_size);
        object->fill(m_collection);
    }
}

void flares::fill(
    manager const& manager, XRay::Editor::property_holder_base* holder, XRay::Editor::property_holder_collection* collection)
{
    XRay::Editor::property_holder_base* properties = holder;
    VERIFY(properties);

    properties->add_property("use", "flares", "this option is responsible for the flares usage", m_use, m_use);

    properties->add_property("shader", "flares", "this option is responsible for flares shader", m_shader.c_str(),
        m_shader, &*manager.m_environment.shader_ids().begin(), manager.m_environment.shader_ids().size(),
        XRay::Editor::property_holder_base::value_editor_tree_view, XRay::Editor::property_holder_base::cannot_enter_text);

    properties->add_property("flares", "flares", "this option is responsible for flares", m_collection);
}
} // namespace editor::environment::suns
