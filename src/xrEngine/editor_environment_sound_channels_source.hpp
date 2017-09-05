////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_sound_channels_source.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment sound channels source class
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_WEATHER_SOUND_CHANNELS_SOURCE_HPP_INCLUDED
#define EDITOR_WEATHER_SOUND_CHANNELS_SOURCE_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"

namespace editor
{
namespace environment
{
namespace sound_channels
{
class source : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    source(shared_str const& source);
    ~source();
    void fill(XRay::Editor::property_holder_collection* collection);
    inline LPCSTR id() const { return m_source.c_str(); }
private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();

private:
    property_holder_type* m_property_holder;
    shared_str m_source;
}; // class source
} // namespace sound_channels
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_WEATHER_SOUND_CHANNELS_SOURCE_HPP_INCLUDED
