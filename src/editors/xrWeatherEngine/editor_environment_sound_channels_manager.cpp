////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_manager.cpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels manager class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_sound_channels_manager.hpp"
#include "property_collection.hpp"
#include "editor_environment_sound_channels_channel.hpp"
#include "editor_environment_detail.hpp"

using sound_channels_manager = editor::environment::sound_channels::manager;

template <>
void property_collection<sound_channels_manager::channel_container_type, sound_channels_manager>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<
    sound_channels_manager::channel_container_type, sound_channels_manager>::create()
{
    using editor::environment::sound_channels::channel;

    auto object = xr_new<channel>(m_holder, generate_unique_id("sound_channel_unique_id_").c_str());
    object->fill(this);
    return (object->object());
}

namespace editor::environment::sound_channels
{
manager::manager() : m_collection(nullptr), m_changed(true)
{
    m_collection = xr_new<collection_type>(&m_channels, this, &m_changed);
}

manager::~manager()
{
    xr_delete(m_collection);
    delete_data(m_channels);
    delete_data(m_channels_ids);
}

void manager::load()
{
    string_path file_name;
    auto config = xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\sound_channels.ltx"), true, true, false);

    VERIFY(m_channels.empty());

    typedef CInifile::Root sections_type;
    const sections_type& sections = config->sections();
    m_channels.reserve(sections.size());

    for (const auto& i : sections)
    {
        auto object = m_channels.emplace_back(xr_new<channel>(*this, i->Name));
        object->load(*config);
        object->fill(m_collection);
    }

    xr_delete(config);
}

void manager::save()
{
    string_path file_name;
    auto config = xr_new<CInifile>(FS.update_path(file_name, "$game_config$", "environment\\sound_channels.ltx"), false, false, true);

    for (const auto& i : m_channels)
        i->save(*config);

    xr_delete(config);
}

void manager::fill(XRay::Editor::property_holder_base* holder)
{
    VERIFY(holder);
    holder->add_property("sound channels", "ambients", "this option is resposible for sound channels", m_collection);
}

manager::channels_ids_type const& manager::channels_ids() const
{
    if (!m_changed)
        return (m_channels_ids);

    m_changed = false;

    delete_data(m_channels_ids);

    m_channels_ids.resize(m_channels.size());

    auto j = m_channels_ids.begin();
    for (const auto& i : m_channels)
        *j++ = xr_strdup(i->id());

    std::sort(m_channels_ids.begin(), m_channels_ids.end(), detail::logical_string_predicate());

    return (m_channels_ids);
}

shared_str manager::unique_id(shared_str const& id) const
{
    if (m_collection->unique_id(id.c_str()))
        return (id);

    return (m_collection->generate_unique_id(id.c_str()));
}
} // namespace editor::environment::sound_channels
