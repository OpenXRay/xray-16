////////////////////////////////////////////////////////////////////////////
// Module : editor_environment_detail.hpp
// Created : 11.01.2008
// Modified : 11.01.2008
// Author : Dmitriy Iassenev
// Description : editor environment detail namespace
////////////////////////////////////////////////////////////////////////////
#pragma once

namespace editor
{
namespace environment
{
namespace detail
{
struct logical_string_predicate
{
    bool operator()(pcstr const& first, pcstr const& second) const;
    bool operator()(shared_str const& first, shared_str const& second) const;
}; // struct logical_string_predicate

shared_str real_path(pcstr folder, pcstr path);

} // namespace detail
} // namespace environment
} // namespace editor
