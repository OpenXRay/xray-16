#include "StdAfx.h"
#include "login_manager.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace gamespy_gp;

SCRIPT_EXPORT(login_manager, (), {
    module(luaState)[class_<login_manager>("login_manager")
                         .def("login", &login_manager::login)
                         .def("stop_login", &login_manager::stop_login)
                         .def("login_offline", &login_manager::login_offline)
                         .def("logout", &login_manager::logout)
                         .def("set_unique_nick", &login_manager::set_unique_nick)
                         .def("stop_setting_unique_nick", &login_manager::stop_setting_unique_nick)
                         .def("save_email_to_registry", &login_manager::save_email_to_registry)
                         .def("get_email_from_registry", &login_manager::get_email_from_registry)
                         .def("save_password_to_registry", &login_manager::save_password_to_registry)
                         .def("get_password_from_registry", &login_manager::get_password_from_registry)
                         .def("save_remember_me_to_registry", &login_manager::save_remember_me_to_registry)
                         .def("get_remember_me_from_registry", &login_manager::get_remember_me_from_registry)
                         .def("save_nick_to_registry", &login_manager::save_nick_to_registry)
                         .def("get_nick_from_registry", &login_manager::get_nick_from_registry)
                         .def("get_current_profile", &login_manager::get_current_profile)
                         .def("forgot_password", &login_manager::forgot_password)

    ];
});

SCRIPT_EXPORT(profile, (), {
    module(
        luaState)[class_<profile>("profile").def("unique_nick", &profile::unique_nick).def("online", &profile::online)];
});

#ifndef LINUX // FIXME!!!
SCRIPT_EXPORT(login_operation_cb, (), {
    module(luaState)[class_<gamespy_gp::login_operation_cb>("login_operation_cb")
                         .def(constructor<>())
                         .def(constructor<gamespy_gp::login_operation_cb::lua_object_type,
                             gamespy_gp::login_operation_cb::lua_function_type>())
                         .def("bind", &gamespy_gp::login_operation_cb::bind)
                         .def("clear", &gamespy_gp::login_operation_cb::clear)];
});
#endif