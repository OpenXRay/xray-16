#include "StdAfx.h"
#include "profile_data_types.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "profile_data_types_script.h"

using namespace luabind;

SCRIPT_EXPORT(profile_data_script_registrator, (), {
    using namespace gamespy_profile;
    module(luaState)[class_<award_data>("award_data")
                         .def_readonly("m_count", &award_data::m_count)
                         .def_readonly("m_last_reward_date", &award_data::m_last_reward_date),

        class_<all_awards_t::value_type>("award_pair_t")
            .def_readonly("first", &all_awards_t::value_type::first)
            .def_readonly("second", &all_awards_t::value_type::second),

        class_<all_best_scores_t::value_type>("best_scores_pair_t")
            .def_readonly("first", &all_best_scores_t::value_type::first)
            .def_readonly("second", &all_best_scores_t::value_type::second)];
});

SCRIPT_EXPORT(store_operation_cb, (), {
    using namespace gamespy_profile;
    module(luaState)[class_<store_operation_cb>("store_operation_cb")
                         .def(constructor<>())
                         .def(constructor<store_operation_cb::lua_object_type,
                             store_operation_cb::lua_function_type>())
                         .def("bind", (store_operation_cb::lua_bind_type)(&store_operation_cb::bind))
                         .def("clear", &store_operation_cb::clear)];
});
