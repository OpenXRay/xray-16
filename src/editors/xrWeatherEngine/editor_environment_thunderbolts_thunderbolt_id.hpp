////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_thunderbolt_id.hpp
// Created : 04.01.2008
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts thunderbolt identifier class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"

namespace editor
{
namespace environment
{
namespace thunderbolts
{
class manager;

class thunderbolt_id : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    thunderbolt_id(manager const& manager, shared_str const& thunderbolt);
    virtual ~thunderbolt_id();
    void fill(XRay::Editor::property_holder_collection* collection);
    inline pcstr id() const { return m_id.c_str(); }
private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();

private:
    pcstr const* xr_stdcall collection();
    u32 xr_stdcall collection_size();

    property_holder_type* m_property_holder;
    manager const& m_manager;
    shared_str m_id;
}; // class thunderbolt_id
} // namespace thunderbolts
} // namespace environment
} // namespace editor
