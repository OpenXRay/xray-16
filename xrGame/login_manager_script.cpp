#include "stdafx.h"
#include "login_manager.h"

using namespace luabind;

#pragma optimize("s",on)

namespace gamespy_gp
{

void login_manager::script_register	(lua_State *L)
{
	module(L)
	[
		class_<login_manager>("login_manager")
			.def("login",							&login_manager::login)
			.def("stop_login",						&login_manager::stop_login)
			.def("login_offline",					&login_manager::login_offline)
			.def("logout",							&login_manager::logout)
			.def("set_unique_nick",					&login_manager::set_unique_nick)
			.def("stop_setting_unique_nick",		&login_manager::stop_setting_unique_nick)
			.def("save_email_to_registry",			&login_manager::save_email_to_registry)
			.def("get_email_from_registry",			&login_manager::get_email_from_registry)
			.def("save_password_to_registry",		&login_manager::save_password_to_registry)
			.def("get_password_from_registry",		&login_manager::get_password_from_registry)
			.def("save_remember_me_to_registry",	&login_manager::save_remember_me_to_registry)
			.def("get_remember_me_from_registry",	&login_manager::get_remember_me_from_registry)
			.def("save_nick_to_registry",			&login_manager::save_nick_to_registry)
			.def("get_nick_from_registry",			&login_manager::get_nick_from_registry)
			.def("get_current_profile",				&login_manager::get_current_profile)
			.def("forgot_password",					&login_manager::forgot_password)

	];
}

void profile::script_register(lua_State *L)
{
	module(L)
	[
		class_<profile>("profile")
			.def("unique_nick",			&profile::unique_nick)
			.def("online",				&profile::online)
	];
}

} //namespace gamespy_gp

DEFINE_MIXED_DELEGATE_SCRIPT(gamespy_gp::login_operation_cb, "login_operation_cb");