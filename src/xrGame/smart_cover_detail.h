////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_detail.h
//	Created 	: 17.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover auxillary namespace
////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SMART_COVER_DETAIL_H_INCLUDED
#define SMART_COVER_DETAIL_H_INCLUDED
#include <limits>
#include "restriction_space.h"
#include "xrCore/_vector3d.h"
#include "xrScriptEngine/script_space_forward.hpp"

namespace smart_cover
{
namespace detail
{
typedef RestrictionSpace::CTimeIntrusiveBase intrusive_base_time;

float parse_float(luabind::adl::object const& table, LPCSTR identifier, float const& min_threshold = flt_min,
    float const& max_threshold = flt_max);
bool parse_float(float& output, luabind::adl::object const& table, LPCSTR identifier,
    float const& min_threshold = flt_min, float const& max_threshold = flt_max);
LPCSTR parse_string(luabind::adl::object const& table, LPCSTR identifier);
void parse_table(luabind::adl::object const& table, LPCSTR identifier, luabind::adl::object& result);
bool parse_bool(luabind::adl::object const& table, LPCSTR identifier);
int parse_int(luabind::adl::object const& table, LPCSTR identifier);
Fvector parse_fvector(luabind::adl::object const& table, LPCSTR identifier);
bool parse_fvector(luabind::adl::object const& table, LPCSTR identifier, Fvector& output);
}; // namespace detail
} // namespace smart_cover

#endif // SMART_COVER_DETAIL_H_INCLUDED
