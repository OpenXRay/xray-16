////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_weathers_time.hpp
// Created : 12.01.2008
// Modified : 12.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment weathers time class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "xrEngine/Environment.h"

namespace editor
{
namespace environment
{
class manager;

namespace weathers
{
class weather;

class time : public CEnvDescriptorMixer, public XRay::Editor::property_holder_holder, private Noncopyable
{
    using inherited = CEnvDescriptorMixer;

public:
    using property_holder_type = XRay::Editor::property_holder_base;

public:
    time(editor::environment::manager* manager, weather const* weather, shared_str const& id);
    virtual ~time();
    void load(const CInifile& config);
    void load_from(shared_str const& id, CInifile& config, shared_str const& new_id);
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* holder);
    inline shared_str const& id() const { return m_identifier; }
    virtual property_holder_type* object() { return m_property_holder; }
    virtual void lerp(
        CEnvironment* parent, CEnvDescriptor& A, CEnvDescriptor& B, float f, CEnvModifier& M, float m_power);

private:
    pcstr const* ambients_collection();
    u32 ambients_collection_size();
    pcstr const* suns_collection();
    u32 suns_collection_size();
    pcstr const* thunderbolts_collection();
    u32 thunderbolts_collection_size();

private:
    pcstr id_getter() const;
    void id_setter(pcstr value);
    float sun_altitude_getter() const;
    void sun_altitude_setter(float value);
    float sun_longitude_getter() const;
    void sun_longitude_setter(float value);
    float sky_rotation_getter() const;
    void sky_rotation_setter(float value);
    float clouds_rotation_getter() const;
    void clouds_rotation_setter(float value);
    float wind_direction_getter() const;
    void wind_direction_setter(float value);
    pcstr ambient_getter() const;
    void ambient_setter(pcstr value);
    pcstr sun_getter() const;
    void sun_setter(pcstr value);
    pcstr thunderbolt_getter() const;
    void thunderbolt_setter(pcstr value);
    pcstr sky_texture_getter() const;
    void sky_texture_setter(pcstr value);
    pcstr clouds_texture_getter() const;
    void clouds_texture_setter(pcstr value);

private:
    shared_str m_ambient;
    shared_str m_sun;
    shared_str m_thunderbolt_collection;

    editor::environment::manager& m_manager;
    weather const* m_weather;
    property_holder_type* m_property_holder;
}; // class time
} // namespace weathers
} // namespace environment
} // namespace editor
