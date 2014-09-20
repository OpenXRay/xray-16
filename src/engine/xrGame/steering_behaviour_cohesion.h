////////////////////////////////////////////////////////////////////////////
//	Module 		: steering_behaviour_cohesion.h
//	Created 	: 07.11.2007
//  Modified 	: 07.11.2007
//	Author		: Dmitriy Iassenev
//	Description : steering behaviour cohesion class
////////////////////////////////////////////////////////////////////////////

#ifndef STEERING_BEHAVIOUR_COHESION_H_INCLUDED
#define STEERING_BEHAVIOUR_COHESION_H_INCLUDED

#include "steering_behaviour_base.h"
#include "smart_cover_detail.h"

class CAI_Rat;

namespace steering_behaviour {

class cohesion :
	public base,
	private debug::make_final<cohesion>,
	private boost::noncopyable 
{
public:
						cohesion	(CAI_Rat const *object);
	virtual	Fvector		direction	();

private:
	typedef base		inherited;
};

} // namespace steering_behaviour

#endif // STEERING_BEHAVIOUR_COHESION_H_INCLUDED