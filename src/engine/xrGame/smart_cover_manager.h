////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_manager.h
//	Created 	: 06.11.2007
//  Modified 	: 06.11.2007
//	Author		: Dmitriy Iassenev
//	Description : smart cover manager class
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_MANAGER_H_INCLUDED
#define SMART_COVER_MANAGER_H_INCLUDED

#include "smart_cover_detail.h"
#include <boost/noncopyable.hpp>

class CAI_Stalker;

namespace smart_cover {

class cover;
class loophole;

namespace transitions {
	class action;
	class processor;
} // namespace transitions

class manager :
	private boost::noncopyable,
	private detail::make_final_debug<manager>
{
public:
	typedef transitions::action		transition_action;
	typedef	transitions::processor	transition_processor;

public:
									manager				(CAI_Stalker *object);
									~manager			();
			loophole const			&enter_loophole		();
	IC		shared_str const		&cover_id			() const;
	IC		void					cover_id			(shared_str const &id);
	IC		bool const				&default_behaviour	() const;
	IC		void					default_behaviour	(bool value);
			bool					enemy_in_fov		() const;
	IC		Fvector const			&fire_position		() const;
	IC		void					fire_position		(Fvector const &value);
	IC		void					invalidate_fire_position	();
	IC		bool					has_fire_position	() const;
		transition_processor const	&transitions_processor	() const;
	IC		loophole const			&get_loophole_by_id	(cover const &cover, shared_str const &loophole_id) const;

public:
			transition_action const	&current_transition	();
			void					go_next_loophole	();
			loophole const			&next_loophole		();

private:
	typedef	xr_vector<shared_str>	LoopholePath;

private:
			void					loophole_path		(cover const &cover, shared_str const &source, shared_str const &target, LoopholePath &path) const;
			void					build_enter_path	();
			void					actualize_path		();
			void					try_actualize_path	();
			transition_action const &transition			(cover const &cover, shared_str const &loophole_id0, shared_str const &loophole_id1) const;
	IC		CAI_Stalker				&object				() const;
	IC		void					enter_cover			(cover const &cover, loophole const &loophole);
	IC		void					exit_cover			();
			bool					fill_enemy_position	(Fvector &position) const;

private:
	LoopholePath					m_path;
	LoopholePath					m_temp_path;
	shared_str						m_cover_id;
	bool							m_default_behaviour;
	Fvector							m_fire_position;
	static const Fvector			ms_invalid_position;
	transition_processor			*m_transitions_processor;
};

} // namespace smart_cover

#include "smart_cover_manager_inline.h"

#endif // SMART_COVER_MANAGER_H_INCLUDED