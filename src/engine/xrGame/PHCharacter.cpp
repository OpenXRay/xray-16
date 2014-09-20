#include "stdafx.h"

#include "phcharacter.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "ExtendedGeom.h"
#include "physicsshellholder.h"

#include "../xrEngine/cl_intersect.h"
#include "../xrEngine/gamemtllib.h"

#include "tri-colliderKNoOPC\__aabb_tri.h"
#include "../3rd party/ode/ode/src/util.h"

CPHCharacter::CPHCharacter(void):
  CPHDisablingTranslational()
{

m_params.acceleration			=0.001f					;
m_params.velocity				=0.0001f				;
m_body							=NULL					;
m_safe_velocity[0]				=0.f					;
m_safe_velocity[1]				=0.f					;	
m_safe_velocity[2]				=0.f					;
m_mean_y		  				=0.f					;	
m_new_restriction_type=m_restriction_type				=rtNone					;
b_actor_movable					=true					;
p_lastMaterialIDX				=&lastMaterialIDX		;
lastMaterialIDX					=GAMEMTL_NONE_IDX		;
injuriousMaterialIDX			=GAMEMTL_NONE_IDX		;
m_creation_step					=u64(-1)				;
b_in_touch_resrtrictor			=false					;
m_current_object_radius			=-1.f					;
}

CPHCharacter::~CPHCharacter(void)
{

}

void	CPHCharacter::FreezeContent()
{
	
	dBodyDisable(m_body);
	CPHObject::FreezeContent();
}
void	CPHCharacter::UnFreezeContent()
{

	dBodyEnable(m_body);
	CPHObject::UnFreezeContent();
}
void	CPHCharacter::getForce(Fvector& force)
{
	if(!b_exist)return;
	force.set(*(Fvector*)dBodyGetForce(m_body));
}
void	CPHCharacter::setForce(const Fvector &force)
{
	if(!b_exist)return;
	dBodySetForce(m_body,force.x,force.y,force.z);
}


void CPHCharacter::get_State(SPHNetState& state)
{
	GetPosition(state.position);
	m_body_interpolation.GetPosition(state.previous_position,0);
	GetVelocity(state.linear_vel);
	getForce(state.force);

	state.angular_vel.set(0.f,0.f,0.f);
	state.quaternion.identity();
	state.previous_quaternion.identity();
	state.torque.set(0.f,0.f,0.f);
//	state.accel = GetAcceleration();
//	state.max_velocity = GetMaximumVelocity();

	if(!b_exist) 
	{
		state.enabled=false;
		return;
	}
	state.enabled=CPHObject::is_active();//!!dBodyIsEnabled(m_body);
}
void CPHCharacter::set_State(const SPHNetState& state)
{
	m_body_interpolation.SetPosition(state.previous_position,0);
	m_body_interpolation.SetPosition(state.position,1);
	SetPosition(state.position);
	SetVelocity(state.linear_vel);
	setForce(state.force);
	
//	SetAcceleration(state.accel);
//	SetMaximumVelocity(state.max_velocity);

	if(!b_exist) return;
	if(state.enabled) 
	{
		Enable();
	};
	if(!state.enabled ) 
	{
		Disable();
	};
	VERIFY2(dBodyStateValide(m_body),"WRONG BODYSTATE WAS SET");
}

void CPHCharacter::Disable()
{

	CPHObject::deactivate();
	dBodyDisable(m_body);
	m_body_interpolation.ResetPositions();
}

void CPHCharacter::Enable()
{
	if(!b_exist) return;
	CPHObject::activate();
	dBodyEnable(m_body);

}







void  CarHitCallback(bool& /**do_colide/**/,dContact& /**c/**/)
{

}

void CPHCharacter::GetSavedVelocity(Fvector& vvel)
{
	
	if(IsEnabled())vvel.set(m_safe_velocity);
	else GetVelocity(vvel);
}

void CPHCharacter::CutVelocity(float l_limit,float /*a_limit*/)
{
	dVector3 limitedl,diffl;
	if(dVectorLimit(dBodyGetLinearVel(m_body),l_limit,limitedl))
	{
		dVectorSub(diffl,limitedl,dBodyGetLinearVel(m_body));
		dBodySetLinearVel(m_body,diffl[0],diffl[1],diffl[2]);
		dBodySetAngularVel(m_body,0.f,0.f,0.f);
		dxStepBody(m_body,fixed_step);
		dBodySetLinearVel(m_body,limitedl[0],limitedl[1],limitedl[2]);
	}
}

const	Fmatrix&	CPHCharacter::XFORM				()							const
{
	return m_phys_ref_object->XFORM();//>renderable.xform;
}
void			CPHCharacter::get_LinearVel		( Fvector& velocity )		const
{
	GetVelocity( velocity );

}
void			CPHCharacter::get_AngularVel		( Fvector& velocity )		const		
{
	velocity.set(0,0,0);
}

const	Fvector	&CPHCharacter::mass_Center		()							const			
{
	return	cast_fv( dBodyGetLinearVel(m_body) );
}