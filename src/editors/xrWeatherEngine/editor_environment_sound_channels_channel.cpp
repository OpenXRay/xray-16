////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_channel.cpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels channel class
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"

#include "editor_environment_sound_channels_channel.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_sound_channels_source.hpp"
#include "editor_environment_sound_channels_manager.hpp"

using sound_channels_channel = editor::environment::sound_channels::channel;

template <>
void property_collection<sound_channels_channel::sound_container_type, sound_channels_channel>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<
    sound_channels_channel::sound_container_type, sound_channels_channel>::create()
{
    using editor::environment::sound_channels::source;

    auto object = xr_new<source>("");
    object->fill(this);
    return (object->object());
}

namespace editor::environment::sound_channels
{
channel::channel(manager const& manager, shared_str const& id)
    : m_manager(manager), m_property_holder(0), m_collection(0)
{
    m_load_section = id;
    m_sound_dist = Fvector2().set(0.f, 0.f);
    m_sound_period = Ivector4().set(0, 0, 0, 0);
    m_collection = xr_new<collection_type>(&m_sounds, this);
}

channel::~channel()
{
    xr_delete(m_collection);
    delete_data(m_sounds);

    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void channel::load(const CInifile& config, pcstr sectionToReadFrom)
{
    inherited::load(config, m_load_section.c_str(), sectionToReadFrom);

    VERIFY(m_sounds.empty());
    pcstr sounds = config.r_string(m_load_section, "sounds");
    string_path sound;
    for (u32 i = 0, n = _GetItemCount(sounds); i < n; ++i)
    {
        auto object = xr_new<source>(_GetItem(sounds, i, sound));
        object->fill(m_collection);
        m_sounds.push_back(object);
    }
}

void channel::save(CInifile& config)
{
    config.w_float(m_load_section.c_str(), "min_distance", m_sound_dist.x);
    config.w_float(m_load_section.c_str(), "max_distance", m_sound_dist.y);
    config.w_s32(m_load_section.c_str(), "period0", m_sound_period.x);
    config.w_s32(m_load_section.c_str(), "period1", m_sound_period.y);
    config.w_s32(m_load_section.c_str(), "period2", m_sound_period.z);
    config.w_s32(m_load_section.c_str(), "period3", m_sound_period.w);

    u32 count = 1;
    for (const auto& i : m_sounds)
        count += xr_strlen(i->id()) + 2;

    pstr temp = (pstr)xr_alloca(count * sizeof(char));
    *temp = '\0';
    for (const auto& i : m_sounds)
    {
        xr_strcat(temp, count, i->id());

        if (&i != &m_sounds.back())
            xr_strcat(temp, count, ", ");
    }

    config.w_string(m_load_section.c_str(), "sounds", temp);
}

pcstr channel::id_getter() const { return (m_load_section.c_str()); }
void channel::id_setter(pcstr value_)
{
    const shared_str value = value_;
    if (m_load_section._get() == value._get())
        return;

    m_load_section = m_manager.unique_id(value);
}

void channel::fill(XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder(m_load_section.c_str(), collection, this);

    typedef XRay::Editor::property_holder_base::string_getter_type string_getter_type;
    string_getter_type string_getter;
    string_getter.bind(this, &channel::id_getter);

    typedef XRay::Editor::property_holder_base::string_setter_type string_setter_type;
    string_setter_type string_setter;
    string_setter.bind(this, &channel::id_setter);

    m_property_holder->add_property("id", "properties", "this option is responsible for sound channel id",
        m_load_section.c_str(), string_getter, string_setter);

    m_property_holder->add_property("minimum distance", "properties",
        "this option is responsible for minimum distance (in meters)", m_sound_dist.x, m_sound_dist.x);

    m_property_holder->add_property("maximum distance", "properties",
        "this option is responsible for maximum distance (in meters)", m_sound_dist.y, m_sound_dist.y);

    m_property_holder->add_property("period 0", "properties",
        "this option is responsible for minimum start time interval (in seconds)", m_sound_period.x, m_sound_period.x);

    m_property_holder->add_property("period 1", "properties",
        "this option is responsible for maximum start time interval (in seconds)", m_sound_period.y, m_sound_period.y);

    m_property_holder->add_property("period 2", "properties",
        "this option is responsible for minimum pause interval (in seconds)", m_sound_period.z, m_sound_period.z);

    m_property_holder->add_property("period 3", "properties",
        "this option is responsible for maximum pause interval (in seconds)", m_sound_period.w, m_sound_period.w);

    m_property_holder->add_property(
        "sounds", "properties", "this option is responsible for sound sources", m_collection);
}

channel::property_holder_type* channel::object() { return (m_property_holder); }
CEnvAmbient::SSndChannel::sounds_type& channel::sounds() { return (inherited::sounds()); }
} // namespace editor::environment::sound_channels
