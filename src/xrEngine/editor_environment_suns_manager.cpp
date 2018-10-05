////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_manager.cpp
// Created : 13.12.2007
// Modified : 13.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment suns manager class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "editor_environment_suns_manager.hpp"
#include "editor_environment_suns_sun.hpp"
#include "Include/editor/ide.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "Common/object_broker.h"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_detail.hpp"

using editor::environment::suns::manager;
using editor::environment::suns::sun;
using editor::environment::detail::logical_string_predicate;

template <>
void property_collection<manager::container_type, manager>::display_name(
    u32 const& item_index, pstr const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id().c_str());
}

template <>
XRay::Editor::property_holder_base* property_collection<manager::container_type, manager>::create()
{
    sun* object = new sun(m_holder, generate_unique_id("sun_unique_id_").c_str());
    object->fill(this);
    return (object->object());
}

manager::manager(::editor::environment::manager* environment)
    : m_environment(*environment), m_collection(0), m_changed(true)
{
    m_collection = new collection_type(&m_suns, this, &m_changed);
}

manager::~manager()
{
    xr_delete(m_collection);

    delete_data(m_suns);
    delete_data(m_suns_ids);
}

void manager::load()
{
    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "suns.ltx"), true, true, false);

    typedef CInifile::Root sections_type;
    sections_type& sections = config->sections();
    m_suns.reserve(sections.size());

    for (const auto &i : sections)
        add(*config, i->Name);

    xr_delete(config);
}

void manager::save()
{
    string_path file_name;
    CInifile* config = new CInifile(FS.update_path(file_name, "$game_config$", "environment" DELIMITER "suns.ltx"), false, false, true);

    for (const auto &i : m_suns)
        i->save(*config);

    xr_delete(config);
}

void manager::add(CInifile& config, shared_str const& section)
{
    struct predicate
    {
        shared_str m_id;

        inline predicate(shared_str const& id) : m_id(id) {}
        inline bool operator()(sun const* const& object) const { return (object->id()._get() == m_id._get()); }
    };

    VERIFY(std::find_if(m_suns.begin(), m_suns.end(), predicate(section)) == m_suns.end());

    sun* object = new sun(*this, section);
    object->load(config);
    object->fill(m_collection);
    m_suns.push_back(object);
}

void manager::fill(XRay::Editor::property_holder_base* holder)
{
    VERIFY(holder);
    holder->add_property("suns", "suns", "this option is responsible for sound channels", m_collection);
}

shared_str manager::unique_id(shared_str const& id) const
{
    if (m_collection->unique_id(id.c_str()))
        return (id);

    return (m_collection->generate_unique_id(id.c_str()));
}

manager::suns_ids_type const& manager::suns_ids() const
{
    if (!m_changed)
        return (m_suns_ids);

    m_changed = false;

    delete_data(m_suns_ids);

    m_suns_ids.resize(m_suns.size() + 1);
    m_suns_ids[0] = xr_strdup("");

    auto j = m_suns_ids.begin() + 1;
    for (const auto &i : m_suns)
        *j++ = xr_strdup(i->id().c_str());

    std::sort(m_suns_ids.begin(), m_suns_ids.end(), logical_string_predicate());

    return (m_suns_ids);
}

struct predicate
{
    shared_str m_id;

    IC predicate(shared_str const& id) : m_id(id) {}
    IC bool operator()(sun* const& sun) const { return (sun->id()._get() == m_id._get()); }
}; // struct predicate

CLensFlareDescriptor* manager::get_flare(shared_str const& id) const
{
    // auto found = std::find_if(m_suns.begin(), m_suns.end(), predicate(id));
    // VERIFY (found != m_suns.end());
    // return ((*found)->);
    return (0);
}

