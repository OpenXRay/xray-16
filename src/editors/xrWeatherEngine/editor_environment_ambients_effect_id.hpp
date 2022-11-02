////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_ambients_effect_id.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment ambients effect identifier class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"

namespace editor
{
namespace environment
{
namespace effects
{
class manager;
} // namespace effects

namespace ambients
{
class effect_id : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    effect_id(effects::manager const& manager, shared_str const& id);
    virtual ~effect_id();
    void fill(XRay::Editor::property_holder_collection* collection);
    inline shared_str const& id() const { return m_id; }

private:
    using property_holder_type = XRay::Editor::property_holder_base;

public:
    virtual property_holder_type* object();

private:
    pcstr const* collection();
    u32 collection_size();

    property_holder_type* m_property_holder;
    effects::manager const& m_manager;
    shared_str m_id;
}; // class effect_id
} // namespace ambients
} // namespace environment
} // namespace editor

