////////////////////////////////////////////////////////////////////////////
//	Module 		: path_manager.h
//	Created 	: 21.03.2002
//  Modified 	: 03.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Path manager
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "path_manager_generic.h"

template <
	typename _Graph,
	typename _DataStorage,
	typename _Parameters,
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>	class CPathManager : 
		public CPathManagerGeneric <
			_Graph,
			_DataStorage,
			_Parameters,
			_dist_type,
			_index_type,
			_iteration_type
		>
{
};

//		path manager parameters
#include "path_manager_params.h"
#include "path_manager_params_flooder.h"
#include "path_manager_params_straight_line.h"
#ifndef AI_COMPILER
#	include "path_manager_params_nearest_vertex.h"
#endif

//		path manager specializations
#include "path_manager_game.h"

#ifndef AI_COMPILER
#	include "path_manager_game_vertex_type.h"
#	include "path_manager_game_vertex.h"
#endif

#include "path_manager_game_level.h"

#include "path_manager_level.h"
#include "path_manager_level_flooder.h"

#ifdef AI_COMPILER
#	include "path_manager_level_straight_line.h"
#else
#	include "path_manager_level_nearest_vertex.h"
#	include "path_manager_solver.h"
#endif