////////////////////////////////////////////////////////////////////////////
//	Module 		: editor_environment_detail.hpp
//	Created 	: 11.01.2008
//  Modified 	: 11.01.2008
//	Author		: Dmitriy Iassenev
//	Description : editor environment detail namespace
////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_ENVIRONMENT_DETAIL_HPP_INCLUDED
#define EDITOR_ENVIRONMENT_DETAIL_HPP_INCLUDED

#ifdef INGAME_EDITOR

namespace editor {
namespace environment {
namespace detail {

struct logical_string_predicate {
	bool		operator()	(LPCSTR const& first, LPCSTR const& second) const;
	bool		operator()	(shared_str const& first, shared_str const& second) const;
}; // struct logical_string_predicate

	shared_str	real_path	(LPCSTR folder, LPCSTR path);

} // namespace detail
} // namespace environment
} // namespace editor

#endif // #ifdef INGAME_EDITOR

#endif // ifndef EDITOR_ENVIRONMENT_DETAIL_HPP_INCLUDED