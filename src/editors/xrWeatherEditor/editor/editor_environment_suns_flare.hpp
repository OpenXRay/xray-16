////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_flare.hpp
// Created : 13.12.2007
// Modified : 04.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns flare class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"

namespace editor
{
namespace environment
{
namespace suns
{
class flare : public XRay::Editor::property_holder_holder, private Noncopyable
{
public:
    flare();
    virtual ~flare();
    void fill(XRay::Editor::property_holder_collection* collection);

public:
    typedef XRay::Editor::property_holder_base property_holder;

public:
    virtual property_holder* object();

private:
    property_holder* m_property_holder;

public:
    shared_str m_texture;
    float m_opacity;
    float m_position;
    float m_radius;
}; // class flare

} // namespace suns
} // namespace environment
} // namespace editor

