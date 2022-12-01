////////////////////////////////////////////////////////////////////////////
//	Module 		: game_path_manager.h
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Game path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "abstract_path_manager.h"
#include "xrAICore/Navigation/game_graph.h"

template <typename _VertexEvaluator, typename _vertex_id_type, typename _index_type>
class CBaseGamePathManager final
    : public CAbstractPathManager<CGameGraph, _VertexEvaluator, _vertex_id_type, _index_type>
{
    typedef CAbstractPathManager<CGameGraph, _VertexEvaluator, _vertex_id_type, _index_type> inherited;

protected:
    IC void before_search(const _vertex_id_type start_vertex_id, const _vertex_id_type dest_vertex_id) override;
    IC void after_search() override;

public:
    IC CBaseGamePathManager(CRestrictedObject* object);
    IC void reinit(const CGameGraph* graph = nullptr);
    IC bool actual() const;
    IC void select_intermediate_vertex() override;
    IC bool completed() const override;
};

#include "game_path_manager_inline.h"
