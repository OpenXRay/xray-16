////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_manager.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels manager class
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
namespace sound_channels
{
class channel;

class manager : private Noncopyable
{
public:
    manager();
    ~manager();
    void load();
    void save();
    void fill(XRay::Editor::property_holder_base* holder);
    shared_str unique_id(shared_str const& id) const;

    using channel_container_type = xr_vector<channel*>;
    using channels_ids_type = xr_vector<pstr>;

    channels_ids_type const& channels_ids() const;

private:
    using property_holder_type = XRay::Editor::property_holder_base;
    using collection_type = property_collection<channel_container_type, manager>;

    channel_container_type m_channels;
    mutable channels_ids_type m_channels_ids;
    property_holder_type* m_property_holder;
    collection_type* m_collection;
    mutable bool m_changed;
}; // class manager
} // namespace sound_channels
} // namespace environment
} // namespace editor

