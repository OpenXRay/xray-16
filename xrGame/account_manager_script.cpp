#include "stdafx.h"
#include "account_manager.h"

using namespace luabind;

#pragma optimize("s",on)

namespace gamespy_gp
{

void account_manager::script_register	(lua_State *L)
{
	module(L)
	[
		class_<account_manager>("account_manager")
			.def("suggest_unique_nicks",			&account_manager::suggest_unique_nicks)
			.def("stop_suggest_unique_nicks",		&account_manager::stop_suggest_unique_nicks)

			.def("get_suggested_unicks",	&account_manager::get_suggested_unicks, return_stl_iterator)
			.def("create_profile",			&account_manager::create_profile)
			.def("delete_profile",			&account_manager::delete_profile)

			.def("is_get_account_profiles_active",	&account_manager::is_get_account_profiles_active)
			.def("get_account_profiles",			&account_manager::get_account_profiles)
			.def("stop_fetching_account_profiles",	&account_manager::stop_fetching_account_profiles)

			.def("get_found_profiles",		&account_manager::get_found_profiles, return_stl_iterator)
			.def("verify_unique_nick",		&account_manager::verify_unique_nick)
			.def("verify_email",			&account_manager::verify_email)
			.def("verify_password",			&account_manager::verify_password)
			.def("get_verify_error_descr",	&account_manager::get_verify_error_descr)
			
			.def("is_email_searching_active",	&account_manager::is_email_searching_active)
			.def("search_for_email",			&account_manager::search_for_email)
			.def("stop_searching_email",		&account_manager::stop_searching_email)
	];
}

} //namespace gamespy_gp

DEFINE_MIXED_DELEGATE_SCRIPT(gamespy_gp::suggest_nicks_cb,		"suggest_nicks_cb");
DEFINE_MIXED_DELEGATE_SCRIPT(gamespy_gp::account_operation_cb,	"account_operation_cb");
DEFINE_MIXED_DELEGATE_SCRIPT(gamespy_gp::account_profiles_cb,	"account_profiles_cb");
DEFINE_MIXED_DELEGATE_SCRIPT(gamespy_gp::found_email_cb,		"found_email_cb");


