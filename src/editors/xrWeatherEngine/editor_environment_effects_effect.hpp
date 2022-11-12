////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_effects_effect.hpp
// Created : 28.12.2007
// Modified : 28.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment effects effect class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "xrEngine/Environment.h"

namespace editor
{
namespace environment
{
class manager;

namespace effects
{
class manager;

class effect : public CEnvAmbient::SEffect, public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    effect(manager const& manager, shared_str const& id);
    virtual ~effect();
    void load(const CInifile& config);
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* collection);
    inline pcstr id() const { return m_id.c_str(); }

private:
    pcstr id_getter() const;
    void id_setter(pcstr value);

    float wind_blast_longitude_getter() const;
    void wind_blast_longitude_setter(float value);

    pcstr sound_getter();
    void sound_setter(pcstr value);

    using property_holder_type = XRay::Editor::property_holder_base;

public:
    virtual property_holder_type* object();

private:
    shared_str m_id;
    property_holder_type* m_property_holder;
    shared_str m_sound;

public:
    manager const& m_manager;
}; // class effect

} // namespace effects
} // namespace environment
} // namespace editor

