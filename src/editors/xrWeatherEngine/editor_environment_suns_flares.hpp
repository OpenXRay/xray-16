////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_flares.hpp
// Created : 26.01.2008
// Modified : 26.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns flares class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "property_collection_forward.hpp"

namespace XRay
{
namespace Editor
{
class property_holder_base;
class property_holder_collection;
}
}

namespace editor
{
namespace environment
{
namespace suns
{
class manager;
class flare;

class flares : private Noncopyable
{
public:
    typedef xr_vector<flare*> flares_type;
    flares();
    virtual ~flares();
    void load(CInifile& config, shared_str const& section);
    void save(CInifile& config, shared_str const& section);
    void fill(manager const& manager, XRay::Editor::property_holder_base* holder, XRay::Editor::property_holder_collection* collection);

private:
    typedef XRay::Editor::property_holder_collection property_holder_collection;

public:
    typedef property_collection<flares_type, flares> collection_type;

private:
    flares_type m_flares;
    shared_str m_shader;
    collection_type* m_collection;
    bool m_use;
}; // class flares

} // namespace suns
} // namespace environment
} // namespace editor

