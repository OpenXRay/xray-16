////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_ambients_ambient.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment ambients ambient class
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
namespace effects
{
class manager;
} // namespace effects

namespace sound_channels
{
class manager;
} // namespace sound_channels

namespace ambients
{
class manager;
class effect_id;
class sound_id;

class ambient : public CEnvAmbient, public XRay::Editor::property_holder_holder, private Noncopyable
{
private:
    typedef CEnvAmbient inherited;

public:
    ambient(manager const& manager, shared_str const& id);
    virtual ~ambient();
    void load(const CInifile& ambients_config, const CInifile& sound_channels_config, const CInifile& effects_config,
        const shared_str& section) override;
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* collection);
    inline shared_str const& id() const { return m_load_section; }
    virtual SEffect* create_effect(const CInifile& config, pcstr id);
    SSndChannel* create_sound_channel(const CInifile& config, pcstr id, pcstr sectionToReadFrom = nullptr) override;
    virtual EffectVec& effects();
    virtual SSndChannelVec& get_snd_channels();

private:
    pcstr xr_stdcall id_getter() const;
    void xr_stdcall id_setter(pcstr value);

public:
    effects::manager const& effects_manager() const;
    sound_channels::manager const& sounds_manager() const;

public:
    typedef xr_vector<effect_id*> effect_container_type;
    typedef property_collection<effect_container_type, ambient> effect_collection_type;

public:
    typedef xr_vector<sound_id*> sound_container_type;
    typedef property_collection<sound_container_type, ambient> sound_collection_type;

private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();

private:
    CInifile* m_config;
    property_holder_type* m_property_holder;
    manager const& m_manager;

    effect_collection_type* m_effects_collection;
    effect_container_type m_effects_ids;

    sound_collection_type* m_sounds_collection;
    sound_container_type m_sound_channels_ids;
}; // class ambient
} // namespace ambients
} // namespace environment
} // namespace editor

