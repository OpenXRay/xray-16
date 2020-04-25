////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_suns_blend.hpp
// Created : 26.01.2008
// Modified : 26.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment suns blend class
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

class blend : private Noncopyable
{
public:
    blend();
    void load(CInifile& config, shared_str const& section);
    void save(CInifile& config, shared_str const& section);
    void fill(manager const& manager, XRay::Editor::property_holder_base* holder, XRay::Editor::property_holder_collection* collection);

private:
    float m_down_time;
    float m_rise_time;
    float m_time;
}; // class blend

} // namespace suns
} // namespace environment
} // namespace editor

