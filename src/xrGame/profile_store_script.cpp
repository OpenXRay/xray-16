#include "StdAfx.h"
#include "profile_store.h"
#include "xrScriptEngine/ScriptExporter.hpp"

using namespace luabind;
using namespace luabind::policy;
using namespace gamespy_profile;

SCRIPT_EXPORT(profile_store, (), {
    module(luaState)[class_<profile_store>("profile_store")
                         .def("load_current_profile", &profile_store::load_current_profile)
                         .def("stop_loading", &profile_store::stop_loading)
                         .def("get_awards", &profile_store::get_awards, return_stl_iterator())
                         .def("get_best_scores", &profile_store::get_best_scores, return_stl_iterator())

                         .enum_("enum_awards_t")[value("at_award_massacre", int(at_award_massacre)),
                             value("at_awards_count", int(at_awards_count))]
                         .enum_("enum_best_score_type")[value("bst_kills_in_row", int(bst_kills_in_row)),
                             value("bst_kinife_kills_in_row", int(bst_kinife_kills_in_row)),
                             value("bst_backstabs_in_row", int(bst_backstabs_in_row)),
                             value("bst_head_shots_in_row", int(bst_head_shots_in_row)),
                             value("bst_eye_kills_in_row", int(bst_eye_kills_in_row)),
                             value("bst_bleed_kills_in_row", int(bst_backstabs_in_row)),
                             value("bst_explosive_kills_in_row", int(bst_head_shots_in_row)),
                             value("bst_score_types_count", int(bst_score_types_count))]];
});
