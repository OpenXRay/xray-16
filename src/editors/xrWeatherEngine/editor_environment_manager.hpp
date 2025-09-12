////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_manager.hpp
// Created : 12.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment manager class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "xrEngine/Environment.h"

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
namespace detail
{
} // namespace detail

namespace suns
{
class manager;
} // namespace suns

namespace effects
{
class manager;
} // namespace effects

namespace levels
{
class manager;
} // namespace levels

namespace sound_channels
{
class manager;
} // namespace sound_channels

namespace ambients
{
class manager;
} // namespace ambients

namespace thunderbolts
{
class manager;
} // namespace thunderbolts

namespace weathers
{
class manager;
} // namespace weathers

class manager : public ::CEnvironment
{
public:
    typedef xr_vector<pcstr> shader_ids_type;
    typedef xr_vector<pcstr> particle_ids_type;
    typedef xr_vector<pcstr> light_animator_ids_type;

public:
    manager();
    virtual ~manager();
    virtual void load_weathers();
    virtual void load();
    virtual void unload();
    virtual void create_mixer();
    CEnvAmbient* AppendEnvAmb(const shared_str& sect, CInifile const* pIni = nullptr) override;
    virtual SThunderboltDesc* thunderbolt_description(const CInifile& config, shared_str const& section);
    virtual SThunderboltCollection* thunderbolt_collection(CInifile const* pIni, CInifile const* thunderbolts, pcstr section);
    virtual SThunderboltCollection* thunderbolt_collection(
        xr_vector<SThunderboltCollection*>& collection, shared_str const& id);
    void save();
    shader_ids_type const& shader_ids() const;
    particle_ids_type const& particle_ids() const;
    light_animator_ids_type const& light_animator_ids() const;

public:
    using property_holder_type = XRay::Editor::property_holder_base;
    using suns_manager_type = editor::environment::suns::manager;
    using levels_manager_type = editor::environment::levels::manager;
    using effects_manager_type = editor::environment::effects::manager;
    using sound_channels_manager_type = editor::environment::sound_channels::manager;
    using ambients_manager_type = editor::environment::ambients::manager;
    using thunderbolts_manager_type = editor::environment::thunderbolts::manager;
    using weathers_manager_type = editor::environment::weathers::manager;

public:
    inline suns_manager_type const& suns() const
    {
        VERIFY(m_suns);
        return (*m_suns);
    }
    inline levels_manager_type const& levels() const
    {
        VERIFY(m_levels);
        return (*m_levels);
    }
    inline effects_manager_type const& effects() const
    {
        VERIFY(m_effects);
        return (*m_effects);
    }
    inline sound_channels_manager_type const& sound_channels() const
    {
        VERIFY(m_sound_channels);
        return (*m_sound_channels);
    }
    inline ambients_manager_type const& ambients() const
    {
        VERIFY(m_ambients);
        return (*m_ambients);
    }
    inline thunderbolts_manager_type const& thunderbolts() const
    {
        VERIFY(m_thunderbolts);
        return (*m_thunderbolts);
    }
    inline weathers_manager_type& weathers() const
    {
        VERIFY(m_weathers);
        return (*m_weathers);
    }

private:
    typedef CEnvironment inherited;

private:
    virtual void load_internal();
    void fill();

private:
    mutable shader_ids_type m_shader_ids;
    mutable particle_ids_type m_particle_ids;
    mutable light_animator_ids_type m_light_animator_ids;

private:
    property_holder_type* m_property_holder;
    suns_manager_type* m_suns;
    levels_manager_type* m_levels;
    effects_manager_type* m_effects;
    sound_channels_manager_type* m_sound_channels;
    ambients_manager_type* m_ambients;
    thunderbolts_manager_type* m_thunderbolts;
    weathers_manager_type* m_weathers;
}; // class manager
} // namespace environment
} // namespace editor
