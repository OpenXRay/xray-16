////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_levels_manager.hpp
// Created : 28.12.2007
// Modified : 28.12.2007
// Author : Dmitriy Iassenev
// Description : editor environment levels manager class
////////////////////////////////////////////////////////////////////////////
#pragma once

#include "Common/Noncopyable.hpp"
#include "xrCore/Containers/AssociativeVector.hpp"

namespace XRay
{
namespace Editor
{
class property_holder_base;
}
}

namespace editor
{
namespace environment
{
namespace weathers
{
class manager;
} // namespace weathers

namespace levels
{
class manager : private Noncopyable
{
public:
    manager(::editor::environment::weathers::manager* environment);
    ~manager();
    void load();
    void save();
    void fill();

private:
    void fill_levels(CInifile& config, pcstr section, pcstr category);

    pcstr const* xr_stdcall collection();
    u32 xr_stdcall collection_size();

private:
    struct predicate
    {
        inline bool operator()(shared_str const& left, shared_str const& right) const
        {
            return (xr_strcmp(left.c_str(), right.c_str()) < 0);
        }
    }; // struct predicate

    typedef AssociativeVector<shared_str, std::pair<pcstr, shared_str>, predicate> levels_container_type;

private:
    levels_container_type m_levels;
    ::editor::environment::weathers::manager& m_weathers;
    CInifile* m_config_single;
    CInifile* m_config_mp;
    XRay::Editor::property_holder_base* m_property_holder;
}; // class levels_manager
} // namespace levels
} // namespace environment
} // namespace editor

