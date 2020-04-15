////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_effects_manager.hpp
// Created : 28.12.2007
// Modified : 28.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment effects manager class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "property_collection_forward.hpp"

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

namespace effects
{
class effect;

class manager : private Noncopyable
{
public:
    manager(editor::environment::manager* environment);
    ~manager();
    void load();
    void save();
    void fill(XRay::Editor::property_holder_base* holder);
    shared_str unique_id(shared_str const& id) const;

    editor::environment::manager& environment() const { return m_environment; }

    using effect_container_type = xr_vector<effect*>;
    using effects_ids_type = xr_vector<pstr>;

    effects_ids_type const& effects_ids() const;

private:
    using property_holder_type = XRay::Editor::property_holder_base;
    using collection_type = property_collection<effect_container_type, manager>;

    effect_container_type m_effects;
    mutable effects_ids_type m_effects_ids;
    editor::environment::manager& m_environment;
    property_holder_type* m_property_holder;
    collection_type* m_collection;
    mutable bool m_changed;
}; // class effects_manager

} // namespace effects
} // namespace environment
} // namespace editor

