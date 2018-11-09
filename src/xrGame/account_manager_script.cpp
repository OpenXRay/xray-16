#include "StdAfx.h"
#include "account_manager.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;
using namespace gamespy_gp;

SCRIPT_EXPORT(account_manager, (), {
    module(luaState)[class_<account_manager>("account_manager")
                         .def("suggest_unique_nicks", &account_manager::suggest_unique_nicks)
                         .def("stop_suggest_unique_nicks", &account_manager::stop_suggest_unique_nicks)

                         .def("get_suggested_unicks", &account_manager::get_suggested_unicks, return_stl_iterator())
                         .def("create_profile", &account_manager::create_profile)
                         .def("delete_profile", &account_manager::delete_profile)

                         .def("is_get_account_profiles_active", &account_manager::is_get_account_profiles_active)
                         .def("get_account_profiles", &account_manager::get_account_profiles)
                         .def("stop_fetching_account_profiles", &account_manager::stop_fetching_account_profiles)

                         .def("get_found_profiles", &account_manager::get_found_profiles, return_stl_iterator())
                         .def("verify_unique_nick", &account_manager::verify_unique_nick)
                         .def("verify_email", &account_manager::verify_email)
                         .def("verify_password", &account_manager::verify_password)
                         .def("get_verify_error_descr", &account_manager::get_verify_error_descr)

                         .def("is_email_searching_active", &account_manager::is_email_searching_active)
                         .def("search_for_email", &account_manager::search_for_email)
                         .def("stop_searching_email", &account_manager::stop_searching_email)];
});

SCRIPT_EXPORT(suggest_nicks_cb, (), {
    using namespace gamespy_gp;
    module(luaState)[class_<suggest_nicks_cb>("suggest_nicks_cb")
                         .def(constructor<>())
                         .def(constructor<suggest_nicks_cb::lua_object_type,
                             suggest_nicks_cb::lua_function_type>())
                         .def("bind", (suggest_nicks_cb::lua_bind_type)(&suggest_nicks_cb::bind))
                         .def("clear", &suggest_nicks_cb::clear)];
});

SCRIPT_EXPORT(account_operation_cb, (), {
    using namespace gamespy_gp;
    module(luaState)[class_<account_operation_cb>("account_operation_cb")
                         .def(constructor<>())
                         .def(constructor<account_operation_cb::lua_object_type,
                             account_operation_cb::lua_function_type>())
                         .def("bind", (account_operation_cb::lua_bind_type)(&account_operation_cb::bind))
                         .def("clear", &account_operation_cb::clear)];
});

SCRIPT_EXPORT(account_profiles_cb, (), {
    using namespace gamespy_gp;
    module(luaState)[class_<account_profiles_cb>("account_profiles_cb")
                         .def(constructor<>())
                         .def(constructor<account_profiles_cb::lua_object_type,
                             account_profiles_cb::lua_function_type>())
                         .def("bind", (account_profiles_cb::lua_bind_type)(&account_profiles_cb::bind))
                         .def("clear", &account_profiles_cb::clear)];
});

SCRIPT_EXPORT(found_email_cb, (), {
    using namespace gamespy_gp;
    module(luaState)[class_<found_email_cb>("found_email_cb")
                         .def(constructor<>())
                         .def(constructor<found_email_cb::lua_object_type,
                             found_email_cb::lua_function_type>())
                         .def("bind", (found_email_cb::lua_bind_type)(&found_email_cb::bind))
                         .def("clear", &found_email_cb::clear)];
});
