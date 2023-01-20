////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path_params_script.cpp
//	Created 	: 30.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path parameters class script export
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "patrol_path_params.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/DebugMacros.hpp" // for THROW3 // XXX: move debug macros to xrCore

Fvector CPatrolPathParams__point(const CPatrolPathParams* params, u32 index)
{
    THROW(params);
    return params->point(index);
}

SCRIPT_EXPORT(CPatrolPathParams, (),
{
    using namespace luabind;

    module(luaState)
    [
        class_<CPatrolPathParams>("patrol")
            .enum_("start")
            [
                value("start", int(ePatrolStartTypeFirst)),
                value("stop", int(ePatrolStartTypeLast)),
                value("nearest", int(ePatrolStartTypeNearest)),
                value("custom", int(ePatrolStartTypePoint)),
                value("next", int(ePatrolStartTypeNext)),
                value("dummy", int(ePatrolStartTypeDummy))
            ]
            .enum_("stop")
            [
                value("stop", int(ePatrolRouteTypeStop)),
                value("continue", int(ePatrolRouteTypeContinue)),
                value("dummy", int(ePatrolRouteTypeDummy))
            ]
            .def(constructor<pcstr>())
            .def(constructor<pcstr, EPatrolStartType>())
            .def(constructor<pcstr, EPatrolStartType, EPatrolRouteType>())
            .def(constructor<pcstr, EPatrolStartType, EPatrolRouteType, bool>())
            .def(constructor<pcstr, EPatrolStartType, EPatrolRouteType, bool, u32>())
            .def("count", &CPatrolPathParams::count)
            .def("level_vertex_id", &CPatrolPathParams::level_vertex_id)
            .def("game_vertex_id", &CPatrolPathParams::game_vertex_id)
            .def("point", &CPatrolPathParams__point)
            .def("name", &CPatrolPathParams::name)
            .def("index", (u32(CPatrolPathParams::*)(pcstr) const)(&CPatrolPathParams::point))
            .def("get_nearest", (u32(CPatrolPathParams::*)(const Fvector&) const)(&CPatrolPathParams::point))
            .def("flag", &CPatrolPathParams::flag)
            .def("flags", &CPatrolPathParams::flags)
            .def("terminal", &CPatrolPathParams::terminal)
    ];
});
