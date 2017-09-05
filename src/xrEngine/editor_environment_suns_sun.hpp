////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_sun.hpp
// Created : 13.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns sun class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SUNS_SUN_HPP_INCLUDED
#define EDITOR_WEATHER_SUNS_SUN_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "xr_efflensflare.h"

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
    LPCSTR xr_stdcall id_getter() const;
    void xr_stdcall id_setter(LPCSTR value);

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

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SUNS_SUN_HPP_INCLUDED
