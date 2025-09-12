////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_channel.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels channel class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection_forward.hpp"
#include "xrEngine/Environment.h"

namespace editor
{
namespace environment
{
namespace sound_channels
{
class source;
class manager;

class channel : public CEnvAmbient::SSndChannel, public XRay::Editor::property_holder_holder, private Noncopyable
{
    using inherited = CEnvAmbient::SSndChannel;

public:
    channel(manager const& manager, shared_str const& id);
    virtual ~channel();
    void load(const CInifile& config, pcstr sectionToReadFrom = nullptr);
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* collection);
    inline pcstr id() const { return m_load_section.c_str(); }
    virtual sounds_type& sounds();

private:
    pcstr id_getter() const;
    void id_setter(pcstr value);

public:
    using sound_container_type = xr_vector<source*>;

private:
    using property_holder_type = XRay::Editor::property_holder_base;
    using collection_type = property_collection<sound_container_type, channel>;

public:
    virtual property_holder_type* object();

private:
    manager const& m_manager;
    property_holder_type* m_property_holder;
    collection_type* m_collection;
    sound_container_type m_sounds;
}; // class channel
} // namespace sound_channels
} // namespace environment
} // namespace editor
