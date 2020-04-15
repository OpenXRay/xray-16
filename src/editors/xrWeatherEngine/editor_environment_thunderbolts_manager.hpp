////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_manager.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts manager class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "property_collection_forward.hpp"

struct SThunderboltDesc;
struct SThunderboltCollection;

namespace XRay
{
namespace Editor
{
class property_holder_base;
}
}

namespace editor
{
namespace environment
{
class manager;

namespace thunderbolts
{
class thunderbolt;
class thunderbolt_id;
class collection;

class manager : private Noncopyable
{
public:
    manager(::editor::environment::manager* environment);
    ~manager();
    void load();
    void save();
    void fill(XRay::Editor::property_holder_base* holder);
    SThunderboltDesc* description(const CInifile& config, shared_str const& section) const;
    SThunderboltCollection* get_collection(shared_str const& section);

public:
    shared_str unique_thunderbolt_id(shared_str const& id) const;
    shared_str unique_collection_id(shared_str const& id) const;

public:
    typedef xr_vector<thunderbolt*> thunderbolt_container_type;
    typedef xr_vector<collection*> collection_container_type;
    typedef xr_vector<pstr> thunderbolts_ids_type;
    typedef xr_vector<pstr> collections_ids_type;

public:
    thunderbolts_ids_type const& thunderbolts_ids() const;
    collections_ids_type const& collections_ids() const;
    ::editor::environment::manager& environment() const;

private:
    void load_thunderbolts();
    void load_collections();
    void save_thunderbolts();
    void save_collections();

private:
    typedef XRay::Editor::property_holder_base property_holder_type;
    typedef property_collection<thunderbolt_container_type, manager> thunderbolt_collection_type;
    typedef property_collection<collection_container_type, manager> collection_collection_type;

private:
    float xr_stdcall altitude_getter() const;
    void xr_stdcall altitude_setter(float value);
    float xr_stdcall longitude_getter() const;
    void xr_stdcall longitude_setter(float value);
    float xr_stdcall tilt_getter() const;
    void xr_stdcall tilt_setter(float value);

private:
    thunderbolt_container_type m_thunderbolts;
    thunderbolt_collection_type* m_thunderbolt_collection;
    bool m_thunderbolts_changed;

    collection_container_type m_collections;
    collection_collection_type* m_collections_collection;
    bool m_collections_changed;

    mutable thunderbolts_ids_type m_thunderbolts_ids;
    mutable collections_ids_type m_collections_ids;
    property_holder_type* m_property_holder;
    ::editor::environment::manager& m_environment;
}; // class manager
} // namespace thunderbolts
} // namespace environment
} // namespace editor

