////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_loophole.h
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Loophole class for smart cover
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_LOOPHOLE_H_INCLUDED
#define SMART_COVER_LOOPHOLE_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover_detail.h"
#include "script_space_forward.h"
#include "associative_vector.h"
#include "graph_abstract.h"
#include "smart_cover_action.h"

namespace smart_cover {

class object;

class loophole : 
	private debug::make_final<loophole>, 
	private boost::noncopyable 
{
private:
	class action_predicate {

	public:
		IC	bool	operator()	(shared_str const &lhs, shared_str const &rhs) const
		{
			return			(lhs._get() < rhs._get());
		}
};
public:
	typedef associative_vector<shared_str, action*, action_predicate>	ActionList;
	typedef xr_vector<shared_str>										TransitionData;
	typedef smart_cover::action::Animations								Animations;
	typedef CGraphAbstract<
				Loki::EmptyType,
				float,
				shared_str,
				TransitionData
			>				TransitionGraph;

private:
	TransitionGraph			m_transitions;
	ActionList				m_actions;
	Fvector					m_fov_position;
	Fvector					m_fov_direction;
	Fvector					m_danger_fov_direction;
	Fvector					m_enter_direction;
	shared_str				m_id;
	float					m_fov;
	float					m_danger_fov;
	float					m_range;
	bool					m_enterable;
	bool					m_usable;
	bool					m_exitable;

public:
							loophole				(luabind::object const &description);
							~loophole				();
	IC	shared_str	const	&id						()		const;
	IC	float		const	&range					()		const;
	IC	float		const	&fov					()		const;
	IC	float		const	&danger_fov				()		const;
	IC	Fvector		const	&fov_position			()		const;
	IC	Fvector		const	&fov_direction			()		const;
	IC	Fvector		const	&danger_fov_direction	()		const;
	IC	Fvector		const	&enter_direction		()		const;
	IC	ActionList	const	&actions				()		const;
	IC	bool		const	&enterable				()		const;
	IC	void				enterable				(bool value);
	IC	bool		const	&usable					()		const;
	IC	bool		const	&exitable				()		const;
	IC	void				exitable				(bool value);
	Animations const		&action_animations		(shared_str const &action_id, shared_str const &animation_id) const;
	TransitionData const	&transition_animations	(shared_str const &action_from, shared_str const &action_to) const;
	IC	bool				is_action_available		(shared_str const &action_id) const;
		void				exit_position			(Fvector &position) const;

private:
		void				add_action				(LPCSTR type, luabind::object const &table);
		void				fill_transitions		(luabind::object const &transitions_table);
};

} // namespace smart_cover

#include "smart_cover_loophole_inline.h"

#endif // SMART_COVER_LOOPHOLE_H_INCLUDED