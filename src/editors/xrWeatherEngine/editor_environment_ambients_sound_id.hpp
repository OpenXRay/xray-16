////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_ambients_sound_id.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment ambients sound identifier class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"

namespace editor
{
namespace environment
{
namespace sound_channels
{
class manager;
} // namespace sound_channels

namespace ambients
{
class sound_id : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    sound_id(sound_channels::manager const& manager, shared_str const& sound);
    virtual ~sound_id();
    void fill(XRay::Editor::property_holder_collection* collection);
    inline shared_str const& id() const { return m_id; }
private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();

private:
    pcstr const* collection();
    u32 collection_size();

private:
    property_holder_type* m_property_holder;
    sound_channels::manager const& m_manager;
    shared_str m_id;
}; // class sound_id
} // namespace ambients
} // namespace environment
} // namespace editor

