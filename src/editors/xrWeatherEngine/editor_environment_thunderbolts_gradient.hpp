////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_thunderbolts_gradient.hpp
// Created : 04.01.2008
// Modified : 10.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment thunderbolts gradient class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "Include/editor/property_holder_base.hpp"
#include "xrEngine/thunderbolt.h"

namespace editor
{
namespace environment
{
class manager;

namespace thunderbolts
{
class gradient : public SThunderboltDesc::SFlare, private Noncopyable
{
public:
    gradient();
    ~gradient();
    void load(const CInifile& config, shared_str const& section_id, pcstr prefix);
    void save(CInifile& config, shared_str const& section_id, pcstr prefix);
    void fill(
        ::editor::environment::manager& environment, pcstr name, pcstr description, XRay::Editor::property_holder_base& holder);

private:
    pcstr shader_getter() const;
    void shader_setter(pcstr value);

    pcstr texture_getter() const;
    void texture_setter(pcstr value);

private:
    XRay::Editor::property_holder_base* m_property_holder;
}; // class gradient

} // namespace thunderbolts
} // namespace environment
} // namespace editor

