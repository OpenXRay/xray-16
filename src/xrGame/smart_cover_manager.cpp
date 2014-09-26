////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_manager.cpp
//	Created 	: 06.11.2007
//  Modified 	: 06.11.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover manager class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_manager.h"
#include "memory_manager.h"
#include "enemy_manager.h"
#include "ai/stalker/ai_stalker.h"
#include "memory_space.h"
#include "cover_point.h"
#include "smart_cover.h"
#include "ai_space.h"
#include "level_graph.h"
#include "graph_engine.h"
#include "cover_evaluators.h"
#include "smart_cover_transition_base.h"
#include "smart_cover_transition_processor.hpp"

using smart_cover::manager;
using smart_cover::cover;
using smart_cover::loophole;
using MemorySpace::CMemoryInfo;

namespace hash_fixed_vertex_manager {
	
IC	u32 to_u32	(shared_str const &string)
{
	const str_value	*get = string._get();
	return			(*(u32 const*)&get);
}

} // namespace hash_fixed_vertex_manager

namespace smart_cover {
	shared_str	transform_vertex(shared_str const &vertex_id, bool const &in);
} // namespace smart_cover

Fvector const manager::ms_invalid_position = Fvector().set(flt_min, flt_min, flt_min);

manager::manager							(CAI_Stalker *object) :
	m_object				(object),
	m_current_cover			(0),
	m_target_cover			(0),
	m_current_loophole		(0),
	m_target_loophole		(0),
	m_current_transition	(0),
	m_cover_id				(""),
	m_default_behaviour		(false)
{
	VERIFY					(object);
	invalidate_fire_position();
	m_transitions_processor	= xr_new<manager::transition_processor>();
}

manager::~manager							()
{
	xr_delete				(m_transitions_processor);
}

bool manager::fill_enemy_position			(Fvector &position) const
{
	CEntityAlive const		*enemy = object().memory().enemy().selected();
	if (!enemy)
		return				(false);

	CMemoryInfo				mem_object = object().memory().memory(enemy);
	position				= mem_object.m_object_params.m_position;

	return					(true);
}

cover const *manager::target_cover			()
{
	m_target_loophole		= 0;
	m_target_cover			= 0;

	Fvector					position;
	if (!fill_enemy_position(position))
		return				(0);

	CCoverPoint const		*point = object().best_cover(position);
	if (!point)
		return				(0);

	if (!point->m_is_smart_cover)
		return				(0);

	m_target_cover			= static_cast<cover const *>(point);
	if (m_current_cover && (m_current_cover != m_target_cover))
		return				(m_target_cover);

	float					value;
	m_target_loophole		= m_target_cover->best_loophole(position, value, m_default_behaviour);
	
//#pragma todo("in case of smart_cover search worked incorrectly. shoul fix it later")
//	if (!m_target_loophole)
//		return				(0);
//
	VERIFY					(m_target_loophole);
	return					(m_target_cover);
}

void manager::loophole_path					(cover const &cover, shared_str const &source_raw, shared_str const &target_raw, LoopholePath &path) const
{
	shared_str				source = transform_vertex(source_raw, true);
	shared_str				target = transform_vertex(target_raw, false);

	typedef GraphEngineSpace::CBaseParameters	CBaseParameters;
	CBaseParameters			parameters(u32(-1),u32(-1),u32(-1));
	path.clear_not_free		();
	R_ASSERT2				(
		ai().graph_engine().search(
			cover.description()->transitions(),
			source,
			target,
			&path,
			parameters
		),
		make_string(
			"cannot build path via loopholes [%s] -> [%s] (cover %s)",
			source_raw.c_str(),
			target_raw.c_str(),
			cover.description()->table_id().c_str()
		)
	);
}

struct loophole_id_predicate {
	shared_str				m_id;

	IC			loophole_id_predicate		(shared_str const &id) :
		m_id				(id)
	{
	}

	IC	bool	operator()					(smart_cover::loophole *loophole) const
	{
		return				(loophole->id()._get() == m_id._get());
	}
};

IC	loophole const &manager::get_loophole_by_id		(cover const &cover, shared_str const &loophole_id) const
{
	typedef cover::Loopholes	Loopholes;
	Loopholes const			&loopholes = cover.description()->loopholes();
	Loopholes::const_iterator	i =
		std::find_if(
			loopholes.begin(),
			loopholes.end(),
			loophole_id_predicate(loophole_id)
		);

	VERIFY					(i != loopholes.end());
	return					(**i);
}

manager::transition_action const &manager::transition	(cover const &cover, shared_str const &loophole_id0, shared_str const &loophole_id1) const
{
	typedef smart_cover::description::TransitionGraph::CEdge	edge_type;
	typedef smart_cover::description::ActionsList				ActionsList;
	typedef smart_cover::transitions::action					action;
	
	edge_type const			*edge = cover.description()->transitions().edge(loophole_id0, loophole_id1);
	VERIFY					(edge);
	ActionsList const		&actions = edge->data();

	struct applicable {
		IC	static bool predicate	(action const * const &action)
		{
			return			(action->applicable());
		}
	};

	ActionsList::const_iterator	i = std::find_if(actions.begin(), actions.end(), &applicable::predicate);
	VERIFY					(i != actions.end());
	return					(**i);
}

void manager::build_enter_path				()
{
	VERIFY					(m_target_cover);

	m_path.clear_not_free	();

	shared_str				target = transform_vertex(m_target_loophole ? m_target_loophole->id() : "", false);
	float					value = flt_max;

	Fvector const			&position = object().Position();
	
	typedef cover::Loopholes	Loopholes;
	Loopholes const				&loopholes = m_target_cover->description()->loopholes();
	Loopholes::const_iterator	I = loopholes.begin();
	Loopholes::const_iterator	E = loopholes.end();
	for ( ; I != E; ++I) {
		if (!(*I)->enterable())
			continue;

		loophole_path		(*m_target_cover, (*I)->id(), target, m_temp_path);
		VERIFY				(!m_temp_path.empty());
		shared_str const	&loophole_id = m_temp_path.front();
		smart_cover::loophole const	&loophole = this->get_loophole_by_id(*m_target_cover, loophole_id);
		float				new_value = m_target_cover->fov_position(loophole).distance_to(position);
		new_value			+= ai().graph_engine().m_string_algorithm->data_storage().get_best().g();
		if (new_value >= value)
			continue;

		value				= new_value;
		m_path.swap			(m_temp_path);
	}

	VERIFY					(!m_path.empty());
	m_path.insert			(m_path.begin(),transform_vertex("", true));
	m_current_transition	= &transition(*m_target_cover, m_path[0], m_path[1]);
}

void manager::actualize_path				()
{
	VERIFY					(m_current_cover || m_target_cover);

	if (!m_current_loophole) {
		build_enter_path	();
		return;
	}

	VERIFY					(m_current_cover);

	shared_str				current_loophole_id = transform_vertex(m_current_loophole ? m_current_loophole->id() : "", true);
	shared_str				target_loophole_id = transform_vertex(m_target_loophole ? m_target_loophole->id() : "", false);
	loophole_path			(
		*m_current_cover,
		current_loophole_id,
		target_loophole_id,
		m_path
	);

	VERIFY					(!m_path.empty());
	if (m_path.size() > 1)
		m_current_transition= &transition(*m_current_cover, m_path[0], m_path[1]);
	else
		m_current_transition= 0;
}

void manager::try_actualize_path			()
{
	if (m_path.empty()) {
		actualize_path		();
		return;
	}

	shared_str				current_loophole_id = transform_vertex(m_current_loophole ? m_current_loophole->id() : "", true);
	if (m_path.front() != current_loophole_id) {
		actualize_path		();
		return;
	}

	shared_str				target_loophole_id = transform_vertex(m_target_loophole ? m_target_loophole->id() : "", false);
	if (m_path.back() == target_loophole_id)
		return;

	actualize_path			();
}

IC	void manager::enter_cover				(cover const &cover, loophole const &loophole)
{
	VERIFY					(!m_current_cover);
	m_current_cover			= &cover;

	VERIFY					(!m_current_loophole);
	m_current_loophole		= &loophole;
}

IC	void manager::exit_cover				()
{
	m_current_cover			= 0;
	m_current_loophole		= 0;
	m_current_transition	= 0;
}

void manager::go_next_loophole				()
{
	try_actualize_path		();

	VERIFY					(!m_path.empty());
	VERIFY					(m_path.size() > 1);

	if (m_path[0]._get() == transform_vertex("", true)._get()) {
		VERIFY				(m_target_cover);
		VERIFY				(m_target_loophole);
		enter_cover			(*m_target_cover, get_loophole_by_id(*m_target_cover, m_path[1]));
		return;
	}

	if (m_path[1]._get() == transform_vertex("", false)._get()) {
		VERIFY				(m_path.size() == 2);
		exit_cover			();
		return;
	}

	m_current_loophole		= &get_loophole_by_id(*m_current_cover, m_path[1]);
}

loophole const &manager::next_loophole	()
{
	VERIFY					(m_current_cover);
	VERIFY					(m_current_loophole);

	try_actualize_path		();
	
	VERIFY					(!m_path.empty());
	VERIFY					(m_path.size() > 1);
	
	return					(get_loophole_by_id(*m_current_cover, m_path[1]));
}

loophole const &manager::enter_loophole	()
{
	VERIFY					(!m_current_cover);
	VERIFY					(!m_current_loophole);
	VERIFY					(m_target_cover);
	VERIFY					(m_target_loophole);

	try_actualize_path		();
	
	VERIFY					(!m_path.empty());
	VERIFY					(m_path.size() > 1);
	VERIFY					(m_path[0]._get() == transform_vertex("", true)._get());
	
	return					(get_loophole_by_id(*m_target_cover, m_path[1]));
}

manager::transition_action const &manager::current_transition	()
{
	VERIFY					((current_cover() != target_cover()) || (m_current_loophole != m_target_loophole));

	try_actualize_path		();

	VERIFY					(m_current_transition);
	return					(*m_current_transition);
}

bool manager::enemy_in_fov										() const
{
	Fvector					position;
	if (!fill_enemy_position(position))
		return				(false);

	if (!m_current_cover)
		return				(false);

	float					(value);
	return					(m_current_cover->best_loophole(position, value, false) != 0);
}

manager::transition_processor const &manager::transitions_processor	() const
{
	return					(*m_transitions_processor);
}