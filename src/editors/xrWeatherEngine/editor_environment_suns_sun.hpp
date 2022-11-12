////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_sun.hpp
// Created : 13.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns sun class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "xrEngine/xr_efflensflare.h"

namespace editor
{
namespace environment
{
namespace suns
{
class flare;
class manager;

class sun : public CLensFlare, public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    sun(manager const& manager, shared_str const& section);
    ~sun();
    void load(CInifile& config);
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* collection);

private:
    pcstr id_getter() const;
    void id_setter(pcstr value);

public:
    inline shared_str const& id() const { return m_id; }
    virtual XRay::Editor::property_holder_base* object();

private:
    shared_str m_id;
    shared_str m_shader;
    shared_str m_texture;
    manager const& m_manager;
    XRay::Editor::property_holder_base* m_property_holder;
    float m_radius;
    bool m_use;
    bool m_ignore_color;
}; // class sun

} // namespace suns
} // namespace environment
} // namespace editor

