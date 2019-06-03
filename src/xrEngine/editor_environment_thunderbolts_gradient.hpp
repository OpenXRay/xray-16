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
#include "thunderbolt.h"

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
    pcstr xr_stdcall shader_getter() const;
    void xr_stdcall shader_setter(pcstr value);

    pcstr xr_stdcall texture_getter() const;
    void xr_stdcall texture_setter(pcstr value);

private:
    XRay::Editor::property_holder_base* m_property_holder;
}; // class gradient

} // namespace thunderbolts
} // namespace environment
} // namespace editor

