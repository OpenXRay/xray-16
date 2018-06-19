////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_collection.hpp
// Created : 10.01.2008
// Modified : 10.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts collection identifier class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "property_collection_forward.hpp"
#include "thunderbolt.h"

namespace editor
{
namespace environment
{
namespace thunderbolts
{
class manager;
class thunderbolt_id;

class collection : public SThunderboltCollection, public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    collection(manager const& manager, shared_str const& id);
    virtual ~collection();
    void load(CInifile& config);
    void save(CInifile& config);
    void fill(XRay::Editor::property_holder_collection* collection);
    inline pcstr id() const { return section.c_str(); }
private:
    pcstr xr_stdcall id_getter() const;
    void xr_stdcall id_setter(pcstr value);

private:
    typedef XRay::Editor::property_holder_base property_holder_type;

public:
    virtual property_holder_type* object();
    typedef xr_vector<thunderbolt_id*> container_type;

private:
    typedef property_collection<container_type, collection> collection_type;

private:
    container_type m_ids;
    collection_type* m_collection;
    property_holder_type* m_property_holder;

public:
    manager const& m_manager;
}; // class collection
} // namespace thunderbolts
} // namespace environment
} // namespace editor

