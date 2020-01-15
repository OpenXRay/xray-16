////////////////////////////////////////////////////////////////////////////
//	Module 		: game_graph_script.cpp
//	Created 	: 02.11.2005
//  Modified 	: 02.11.2005
//	Author		: Dmitriy Iassenev
//	Description : Game graph class script export
////////////////////////////////////////////////////////////////////////////

#include "pch.hpp"
#include "game_graph.h"
#include "xrScriptEngine/ScriptExporter.hpp"
#include "xrScriptEngine/DebugMacros.hpp" // for THROW // XXX: move debug macros to xrCore
#include "AISpaceBase.hpp"

using namespace luabind;
using namespace luabind::policy;

const CGameGraph* get_game_graph() { return &GEnv.AISpace->game_graph(); }
const CGameGraph::CHeader* get_header(const CGameGraph* self_) { return (&self_->header()); }
bool get_accessible1(const CGameGraph* self_, const u32& vertex_id) { return (self_->accessible(vertex_id)); }
void get_accessible2(const CGameGraph* self_, const u32& vertex_id, bool value) { self_->accessible(vertex_id, value); }
Fvector CVertex__level_point(const CGameGraph::CGameVertex* vertex)
{
    THROW(vertex);
    return (vertex->level_point());
}

Fvector CVertex__game_point(const CGameGraph::CGameVertex* vertex)
{
    THROW(vertex);
    return (vertex->game_point());
}

GameGraph::LEVEL_MAP const& get_levels(CGameGraph const* graph)
{
    THROW(graph);
    return graph->header().levels();
}

SCRIPT_EXPORT(CGameGraph, (), {
    typedef CGameGraph::CGameVertex CGameVertex;
    module(luaState)[class_<GameGraph::LEVEL_MAP::value_type>("GameGraph__LEVEL_MAP__value_type")
                         .def_readonly("id", &GameGraph::LEVEL_MAP::value_type::first)
                         .def_readonly("level", &GameGraph::LEVEL_MAP::value_type::second),

        def("game_graph", &get_game_graph),

        class_<CGameGraph>("CGameGraph")
            .def("accessible", &get_accessible1)
            .def("accessible", &get_accessible2)
            .def("valid_vertex_id", &CGameGraph::valid_vertex_id)
            .def("vertex", &CGameGraph::vertex)
            .def("vertex_id", &CGameGraph::vertex_id)
            .def("levels", &get_levels, return_stl_iterator()),

        class_<CGameVertex>("GameGraph__CVertex")
            .def("level_point", &CVertex__level_point)
            .def("game_point", &CVertex__game_point)
            .def("level_id", &CGameVertex::level_id)
            .def("level_vertex_id", &CGameVertex::level_vertex_id)];
});
