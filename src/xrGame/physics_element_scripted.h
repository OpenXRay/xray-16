#pragma once
#include "../xrphysics/iphysics_scripted.h"
#include "../xrphysics/physicsshell.h"
#include "script_export_space.h"

class CPhysicsElement;
class cphysics_element_scripted:
public cphysics_game_scripted<CPhysicsElement>
{
public:
	cphysics_element_scripted(CPhysicsElement* imp ):cphysics_game_scripted<CPhysicsElement>(imp){}

	void	applyForce			( float x, float y, float z )	{ physics_impl().applyForce( x, y, z ); }
	bool	isBreakable			( )								{ return physics_impl(). isBreakable( ); }
	void	get_LinearVel		( Fvector& velocity )	const	{ physics_impl().get_LinearVel( velocity ); }
	void	get_AngularVel		( Fvector& velocity )	const	{ physics_impl().get_AngularVel( velocity ); }


	float	getMass				()								{ return physics_impl().getMass	();	}
	float	getDensity			()								{ return physics_impl().getDensity(); }
	float	getVolume			()								{ return physics_impl().getVolume(); }

	void	Fix					()								{ physics_impl().Fix();	}				
	void	ReleaseFixed		()								{ physics_impl().ReleaseFixed(); }
	bool	isFixed				()								{ return physics_impl().isFixed(); }
	void	GetGlobalTransformDynamic(Fmatrix* m)		const	{ physics_impl().GetGlobalTransformDynamic( m ); }

	DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(cphysics_element_scripted)
#undef script_type_list
#define script_type_list save_type_list(cphysics_element_scripted)