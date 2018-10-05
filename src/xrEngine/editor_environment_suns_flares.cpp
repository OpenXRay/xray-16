////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_flares.cpp
// Created : 26.01.2008
// Modified : 26.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns flares class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "editor_environment_suns_flares.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection.hpp"
#include "editor_environment_suns_flare.hpp"
#include "editor_environment_suns_manager.hpp"
#include "editor_environment_manager.hpp"

using editor::environment::suns::flares;
using editor::environment::suns::manager;
using editor::environment::suns::flare;

template <>
void property_collection<flares::flares_type, flares>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    float position = m_container[item_index]->m_position;
    xr_sprintf(buffer, buffer_size, "flare [%f]", position);
}

template <>
XRay::Editor::property_holder_base* property_collection<flares::flares_type, flares>::create()
{
    flare* object = new flare();
    object->fill(this);
    return (object->object());
}

flares::flares() : m_use(false), m_shader(""), m_collection(0) { m_collection = new collection_type(&m_flares, this); }
flares::~flares()
{
    xr_delete(m_collection);
    delete_data(m_flares);
}

void flares::load(CInifile& config, shared_str const& section)
{
    m_use = !!READ_IF_EXISTS(&config, r_bool, section, "flares", true);
    m_shader = READ_IF_EXISTS(&config, r_string, section, "flare_shader", "effects" DELIMITER "flare");

    if (!m_use)
        return;

    shared_str flare_opacity =
        READ_IF_EXISTS(&config, r_string, section, "flare_opacity", "0.340, 0.260, 0.500, 0.420, 0.260, 0.260");
    shared_str flare_position =
        READ_IF_EXISTS(&config, r_string, section, "flare_position", "1.300, 1.000, 0.500, -0.300, -0.600, -1.000");
    shared_str flare_radius =
        READ_IF_EXISTS(&config, r_string, section, "flare_radius", "0.080, 0.120, 0.040, 0.080, 0.120, 0.300");
    shared_str flare_textures = READ_IF_EXISTS(&config, r_string, section, "flare_textures",
        "fx" DELIMITER "fx_flare1.tga, fx" DELIMITER "fx_flare2.tga, fx" DELIMITER "fx_flare2.tga, fx" DELIMITER "fx_flare2.tga, fx" DELIMITER "fx_flare3.tga, "
        "fx" DELIMITER "fx_flare1.tga");

    u32 opacity_count = _GetItemCount(flare_opacity.c_str());
    u32 position_count = _GetItemCount(flare_position.c_str());
    u32 radius_count = _GetItemCount(flare_radius.c_str());
    u32 texture_count = _GetItemCount(flare_textures.c_str());

    u32 min_flare_count = _min(_min(_min(opacity_count, position_count), radius_count), texture_count);
    u32 max_flare_count = _max(_max(_max(opacity_count, position_count), radius_count), texture_count);

    u32 max_string_count =
        _max(_max(_max(flare_opacity.size(), flare_position.size()), flare_radius.size()), flare_textures.size()) + 1;

    if (min_flare_count != max_flare_count)
        Msg("! flare count for sun [%s] is setup incorrectly. only %d flares are correct", section.c_str(),
            min_flare_count);

    u32 const buffer_size = max_string_count * sizeof(char);
    pstr result = (pstr)_alloca(buffer_size);
    for (u32 i = 0; i < min_flare_count; ++i)
    {
        flare* object = new flare();
        object->m_opacity = (float)atof(_GetItem(flare_opacity.c_str(), i, result, buffer_size));
        object->m_position = (float)atof(_GetItem(flare_position.c_str(), i, result, buffer_size));
        object->m_radius = (float)atof(_GetItem(flare_radius.c_str(), i, result, buffer_size));
        object->m_texture = _GetItem(flare_textures.c_str(), i, result, buffer_size);
        object->fill(m_collection);
        m_flares.push_back(object);
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

