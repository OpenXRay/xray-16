////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_gradient.hpp
// Created : 26.01.2008
// Modified : 26.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns gradient class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"

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

class gradient : private Noncopyable
{
public:
    gradient();
    void load(CInifile& config, shared_str const& section);
    void save(CInifile& config, shared_str const& section);
    void fill(manager const& manager, XRay::Editor::property_holder_base* holder, XRay::Editor::property_holder_collection* collection);

private:
    bool use_getter();
    void use_setter(bool value);

private:
    bool m_use;
    float m_opacity;
    float m_radius;
    shared_str m_shader;
    shared_str m_texture;
}; // class gradient

} // namespace suns
} // namespace environment
} // namespace editor

