////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_weathers_weather.cpp
// Created : 11.01.2008
// Modified : 11.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment weathers weather class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "editor_environment_weathers_weather.hpp"
#include "ide.hpp"
#include "editor_environment_weathers_manager.hpp"
#include "property_collection.hpp"
#include "editor_environment_weathers_time.hpp"
#include "editor_environment_manager.hpp"

using editor::environment::weathers::weather;
using editor::environment::weathers::manager;
using editor::environment::weathers::time;

template <>
void property_collection<weather::container_type, weather>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
XRay::Editor::property_holder_base* property_collection<weather::container_type, weather>::create()
{
    using ::editor::environment::weathers::time;
    time* object = new time(&m_holder.m_manager, &m_holder, m_holder.generate_unique_id().c_str());
    object->fill(this);
    return (object->object());
}

weather::weather(editor::environment::manager* manager, shared_str const& id)
    : m_manager(*manager), m_id(id), m_property_holder(0), m_collection(0)
{
    m_collection = new collection_type(&m_times, this);
}

weather::~weather()
{
    xr_delete(m_collection);
    delete_data(m_times);

    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void weather::load()
{
    string_path file_name;
    FS.update_path(file_name, "$game_weathers$", m_id.c_str());
    xr_strcat(file_name, ".ltx");
    CInifile* config = CInifile::Create(file_name);

    m_manager.WeatherCycles[m_id].clear();

    typedef CInifile::Root sections_type;
    sections_type& sections = config->sections();
    m_times.reserve(sections.size());

    for (const auto &i : sections)
    {
        time* object = new time(&m_manager, this, i->Name);
        object->load(*config);
        object->fill(m_collection);
        m_times.push_back(object);
        m_manager.WeatherCycles[m_id].push_back(object);
    }

    CInifile::Destroy(config);
}

void weather::save()
{
    string_path file_name;
    FS.update_path(file_name, "$game_weathers$", m_id.c_str());
    xr_strcat(file_name, ".ltx");
    CInifile* config = new CInifile(file_name, false, false, true);

    for (const auto &i : m_times)
        i->save(*config);

    CInifile::Destroy(config);
}

pcstr weather::id_getter() const { return (m_id.c_str()); }
void weather::id_setter(pcstr value_)
{
    shared_str value = value_;
    if (m_id._get() == value._get())
        return;

    m_id = m_manager.weathers().unique_id(value);
}

void weather::fill(XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder(m_id.c_str(), collection, this);

    typedef XRay::Editor::property_holder_base::string_getter_type string_getter_type;
    string_getter_type string_getter;
    string_getter.bind(this, &weather::id_getter);

    typedef XRay::Editor::property_holder_base::string_setter_type string_setter_type;
    string_setter_type string_setter;
    string_setter.bind(this, &weather::id_setter);

    m_property_holder->add_property("id", "properties", "this option is resposible for weather identifier",
        m_id.c_str(), string_getter, string_setter);
    m_property_holder->add_property("times", "properties", "this option is resposible for times", m_collection);
}

static inline bool is_digit(char const& test)
{
    if (test < '0')
        return (false);

    if (test > '9')
        return (false);

    return (true);
}

bool weather::valid_id(shared_str const& id_) const
{
    pcstr id = id_.c_str();
    if (!is_digit(id[0]))
        return (false);

    if (!is_digit(id[1]))
        return (false);

    if (id[2] != ':')
        return (false);

    if (!is_digit(id[3]))
        return (false);

    if (!is_digit(id[4]))
        return (false);

    if (id[5] != ':')
        return (false);

    if (!is_digit(id[6]))
        return (false);

    if (!is_digit(id[7]))
        return (false);

    return (true);
}

shared_str weather::unique_id(shared_str const& current, shared_str const& id) const
{
    if (!valid_id(id))
        return (current);

    if (m_collection->unique_id(id.c_str()))
        return (id);

    return (generate_unique_id(id));
}

bool weather::try_hours(u32& hours, u32& minutes, u32& seconds, shared_str& result) const
{
    for (u32 i = hours + 1; i < 24; ++i)
    {
        string16 temp;
        xr_sprintf(temp, "%02d:%02d:%02d", i, minutes, seconds);
        if (!m_collection->unique_id(temp))
            continue;

        result = temp;
        return (true);
    }

    return (false);
}

bool weather::try_minutes(u32& hours, u32& minutes, u32& seconds, shared_str& result) const
{
    for (u32 i = minutes + 1; i < 60; ++i)
    {
        string16 temp;
        xr_sprintf(temp, "%02d:%02d:%02d", hours, i, seconds);
        if (!m_collection->unique_id(temp))
            continue;

        result = temp;
        return (true);
    }

    return (false);
}

shared_str weather::try_all(u32& hours_, u32& minutes_, u32& seconds_) const
{
    for (u32 hours = hours_; hours < 24; ++hours)
        for (u32 minutes = minutes_; minutes < 60; ++minutes)
            for (u32 seconds = seconds_ + 1; seconds < 60; ++seconds)
            {
                string16 temp;
                xr_sprintf(temp, "%02d:%02d:%02d", hours, minutes, seconds);
                if (!m_collection->unique_id(temp))
                    continue;

                return (temp);
            }
    return ("can not generate weather id");
}

shared_str weather::generate_unique_id(shared_str const& start) const
{
    string16 id;
    xr_strcpy(id, start.c_str());

    VERIFY(xr_strlen(id) == 8);
    VERIFY(is_digit(id[0]));
    VERIFY(is_digit(id[1]));
    VERIFY(id[2] == ':');
    VERIFY(is_digit(id[3]));
    VERIFY(is_digit(id[4]));
    VERIFY(id[5] == ':');
    VERIFY(is_digit(id[6]));
    VERIFY(is_digit(id[7]));

    u32 hours, minutes, seconds;
    sscanf(id, "%02d:%02d:%02d", &hours, &minutes, &seconds);

    shared_str result;

    if (try_hours(hours, minutes, seconds, result))
        return (result);

    if (try_minutes(hours, minutes, seconds, result))
        return (result);

    return (try_all(hours, minutes, seconds));
}

shared_str weather::generate_unique_id() const
{
    if (m_times.empty())
        return ("00:00:00");

    return (generate_unique_id(m_times.back()->id()));
}

bool weather::save_time_frame(shared_str const& frame_id, char* buffer, u32 const& buffer_size)
{
    for (const auto &i : m_times)
    {
        if (frame_id._get() != i->id()._get())
            continue;

        CInifile temp(nullptr, false, false, false);
        i->save(temp);

        CMemoryWriter writer;
        temp.save_as(writer);
        if (writer.size() > buffer_size)
            return (false);

        writer.seek(0);
        memcpy(buffer, writer.pointer(), writer.size());
        return (true);
    }

    return (false);
}

bool weather::paste_time_frame(shared_str const& frame_id, char const* buffer, u32 const& buffer_size)
{
    for (const auto &i : m_times)
    {
        if (frame_id._get() != i->id()._get())
            continue;

        IReader reader(const_cast<char*>(buffer), buffer_size);
        CInifile temp(&reader);
        if (temp.sections().empty())
            return (false);

        i->load_from((*temp.sections().begin())->Name, temp, shared_str(i->id()));
        return (true);
    }

    return (false);
}

bool weather::add_time_frame(char const* buffer, u32 const& buffer_size)
{
    IReader reader(const_cast<char*>(buffer), buffer_size);
    CInifile temp(&reader);
    if (temp.sections().empty())
        return (false);

    shared_str const& section = (*temp.sections().begin())->Name;
    for (const auto &i : m_times)
        if (section._get() == i->id()._get())
            return (false);

    time* object = new time(&m_manager, this, section);
    object->load(temp);
    object->fill(m_collection);

    struct id
    {
        static inline bool predicate(time* const& time, shared_str const& id)
        {
            return (xr_strcmp(time->id(), id) < 0);
        }
    }; // struct id

    auto found = std::lower_bound(m_times.begin(), m_times.end(), section, &id::predicate);

    u32 index = u32(found - m_times.begin());
    m_times.insert(found, object);
    m_manager.WeatherCycles[m_id].insert(m_manager.WeatherCycles[m_id].begin() + index, object);
    return (true);
}

void weather::reload_time_frame(shared_str const& frame_id)
{
    string_path file_name;
    FS.update_path(file_name, "$game_weathers$", m_id.c_str());
    xr_strcat(file_name, ".ltx");
    CInifile* config = CInifile::Create(file_name);

    for (const auto &i : m_times)
    {
        if (frame_id._get() != i->id()._get())
            continue;

        if (!config->section_exist(i->id()))
            return;

        i->load_from(i->id(), *config, i->id());
        CInifile::Destroy(config);
        return;
    }

    CInifile::Destroy(config);
}

void weather::reload()
{
    delete_data(m_times);
    load();
}

