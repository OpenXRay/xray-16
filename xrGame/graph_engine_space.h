////////////////////////////////////////////////////////////////////////////
//	Module 		: graph_engine_space.h
//	Created 	: 21.03.2002
//  Modified 	: 26.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Graph engine
////////////////////////////////////////////////////////////////////////////

#pragma once

template <
	typename _condition_type,
	typename _value_type
>
class COperatorConditionAbstract;

template <
	typename _world_property
>
class CConditionState;

template <
	typename _world_property,
	typename _edge_value_type
>
class COperatorAbstract;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SBaseParameters;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SFlooder ;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SStraightLineParams;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SNearestVertex;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SGameLevel;

template <
	typename _dist_type,
	typename _index_type,
	typename _iteration_type
>
struct SGameVertex;

namespace GraphEngineSpace {
	typedef float		_dist_type;
	typedef u32			_index_type;
	typedef u32			_iteration_type;
	typedef u16			_solver_dist_type;
	typedef u32			_solver_condition_type;
	typedef bool		_solver_value_type;

	struct CSolverConditionValue {
		_solver_condition_type	m_condition;
		_solver_value_type		m_value;

		IC				CSolverConditionValue	(const _solver_condition_type &condition, const _solver_value_type &value)
		{
			m_condition			= condition;
			m_value				= value;
		}

		IC		bool	operator==				(const _solver_condition_type &condition) const
		{
			return				(condition == m_condition);
		}
	};

	typedef xr_vector<CSolverConditionValue>	CSolverConditionStorage;

	typedef COperatorConditionAbstract<
				_solver_condition_type,
				_solver_value_type
			>			CWorldProperty;

	typedef CConditionState<
				CWorldProperty
			>			CWorldState;

	typedef COperatorAbstract<
				CWorldProperty,
				_solver_dist_type
			>			CWorldOperator;

	typedef CWorldState	_solver_index_type;
	typedef u32			_solver_edge_type;

	typedef SBaseParameters<
				_solver_dist_type,
				_solver_index_type,
				_iteration_type
			>		CSolverBaseParameters;
	typedef SBaseParameters<
				_dist_type,
				_index_type,
				_iteration_type
			>		CBaseParameters;

	typedef SFlooder<
				_dist_type,
				_index_type,
				_iteration_type
			>		CFlooder;
	typedef SStraightLineParams<
				_dist_type,
				_index_type,
				_iteration_type
			>		CStraightLineParams;
	typedef SNearestVertex<
				_dist_type,
				_index_type,
				_iteration_type
			>		CNearestVertexParameters;
	typedef SGameLevel<
				_dist_type,
				_index_type,
				_iteration_type
			>		CGameLevelParams;
	typedef SGameVertex<
				_dist_type,
				_index_type,
				_iteration_type
			>		CGameVertexParams;
};
