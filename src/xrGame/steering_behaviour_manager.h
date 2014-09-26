////////////////////////////////////////////////////////////////////////////
//	Module 		: steering_behaviour_manager.h
//	Created 	: 07.11.2007
//  Modified 	: 07.11.2007
//	Author		: Dmitriy Iassenev
//	Description : steering behaviour manager class
////////////////////////////////////////////////////////////////////////////

#ifndef STEERING_BEHAVIOUR_MANAGER_H_INCLUDED
#define STEERING_BEHAVIOUR_MANAGER_H_INCLUDED

#include <boost/noncopyable.hpp>
#include "smart_cover_detail.h"

class CAI_Rat;

// #include "../../../steering_behaviour_manager.h"
// #include "../../../steering_behaviour_cohesion.h"
// #include "../../../steering_behaviour_alignment.h"
// #include "../../../steering_behaviour_separation.h"	private boost::noncopyable,
	private debug::make_final<manager>,
	private boost::noncopyable 
{
public:
					manager			(CAI_Rat const *object);
					~manager		();
			void	add				(base *behaviour, float const &factor);
			void	remove			(base *behaviour);
			Fvector	new_position	(float const &time_delta);

private:
			void	clear			();

private:
	typedef xr_map<base*,float>		Behaviours;

private:
	Behaviours		m_behaviours;
};

} // namespace steering_behaviour

#endif // STEERING_BEHAVIOUR_MANAGER_H_INCLUDED