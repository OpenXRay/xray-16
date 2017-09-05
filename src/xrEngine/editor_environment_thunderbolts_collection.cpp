////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_collection.cpp
// Created : 10.01.2008
// Modified : 10.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts collection identifier class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef INGAME_EDITOR
#include "editor_environment_thunderbolts_collection.hpp"
#include "ide.hpp"
#include "property_collection.hpp"
#include "editor_environment_thunderbolts_thunderbolt_id.hpp"
#include "editor_environment_thunderbolts_manager.hpp"

using editor::environment::thunderbolts::thunderbolt_id;
using editor::environment::thunderbolts::collection;
using editor::environment::thunderbolts::manager;

template <>
void property_collection<collection::container_type, collection>::display_name(
    u32 const& item_index, LPSTR const& buffer, u32 const& buffer_size)
{
    xr_strcpy(buffer, buffer_size, m_container[item_index]->id());
}

template <>
XRay::Editor::property_holder_base* property_collection<collection::container_type, collection>::create()
{
    thunderbolt_id* object = new thunderbolt_id(m_holder.m_manager, "");
    object->fill(this);
    return (object->object());
}

collection::collection(manager const& manager, shared_str const& id)
    : m_manager(manager), m_collection(0), m_property_holder(0)
{
    section = id;
    m_collection = new collection_type(&m_ids, this);
}

collection::~collection()
{
    xr_delete(m_collection);
    delete_data(m_ids);

    palette.clear();

    if (!Device.editor())
        return;

    ::ide().destroy(m_property_holder);
}

void collection::load(CInifile& config)
{
    CInifile::Sect& items = config.r_section(section);
    m_ids.reserve(items.Data.size());
    typedef CInifile::Items items_type;
    items_type::const_iterator i = items.Data.begin();
    items_type::const_iterator e = items.Data.end();
    for (; i != e; ++i)
    {
        thunderbolt_id* object = new thunderbolt_id(m_manager, (*i).first);
        object->fill(m_collection);
        m_ids.push_back(object);

        palette.push_back(m_manager.description(config, (*i).first));
    }
}

void collection::save(CInifile& config)
{
    container_type::const_iterator i = m_ids.begin();
    container_type::const_iterator e = m_ids.end();
    for (; i != e; ++i)
        config.w_string(section.c_str(), (*i)->id(), "");
}

LPCSTR collection::id_getter() const { return (section.c_str()); }
void collection::id_setter(LPCSTR value_)
{
    shared_str value = value_;
    if (section._get() == value._get())
        return;

    section = m_manager.unique_collection_id(value);
}

void collection::fill(XRay::Editor::property_holder_collection* collection)
{
    VERIFY(!m_property_holder);
    m_property_holder = ::ide().create_property_holder(section.c_str());

    typedef XRay::Editor::property_holder_base::string_getter_type string_getter_type;
    string_getter_type string_getter;
    string_getter.bind(this, &collection::id_getter);

    typedef XRay::Editor::property_holder_base::string_setter_type string_setter_type;
    string_setter_type string_setter;
    string_setter.bind(this, &collection::id_setter);

    m_property_holder->add_property("id", "properties", "this option is responsible for collection id", section.c_str(),
        string_getter, string_setter);

    m_property_holder->add_property(
        "thunderbolts", "properties", "this option is responsible for thunderbolts", m_collection);
}

XRay::Editor::property_holder_base* collection::object() { return m_property_holder; }
#endif // #ifdef INGAME_EDITOR
