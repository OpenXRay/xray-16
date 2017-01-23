////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_action.h
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Action class for smart cover
////////////////////////////////////////////////////////////////////////////

#ifndef SMART_COVER_ACTION_H_INCLUDED
#define SMART_COVER_ACTION_H_INCLUDED

#include "Common/Noncopyable.hpp"
#include "smart_cover_detail.h"
#include "xrScriptEngine/script_space_forward.hpp"
#include "ai_monster_space.h"
#include "xrCore/Containers/AssociativeVector.hpp"

namespace smart_cover{

class action final : private Noncopyable
{
private:
	class animation_predicate {

	public:
		IC	bool	operator()	(shared_str const &lhs, shared_str const &rhs) const
		{
			return			(lhs._get() < rhs._get());
		}
};

public:
	typedef xr_vector<shared_str>												Animations;
	typedef AssociativeVector<shared_str, Animations*, animation_predicate>	AnimationList;

private:
	AnimationList			m_animations;

	bool					m_movement;
	Fvector					m_target_position;

public:
							action				(luabind::adl::object const &description);
							~action				();
	IC	bool		const	&movement			() const;
	IC	Fvector		const	&target_position	() const;
	IC	Animations	const	&animations			(shared_str const& cover_id, shared_str const &id) const;

private:
		void				add_animation		(LPCSTR animation_type, luabind::adl::object const &table);
};

} // namespace smart_cover

#include "smart_cover_action_inline.h"

#endif // SMART_COVER_ACTION_H_INCLUDED