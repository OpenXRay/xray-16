#pragma once
#include "PHSimpleCharacter.h"
class CPHAICharacter : public CPHSimpleCharacter
{
	typedef CPHSimpleCharacter	inherited;


	bool	m_forced_physics_control;
public:
							CPHAICharacter						();
	virtual CPHAICharacter	*CastAICharacter					()																					{return this;}
	//virtual		void		SetPosition							(const Fvector &pos);
	//virtual		void		SetDesiredPosition					(const Fvector& pos)																{m_vDesiredPosition.set(pos)	;}
	//virtual		void		GetDesiredPosition					(Fvector& dpos)																		{dpos.set(m_vDesiredPosition)	;}
	virtual		void		ValidateWalkOn						()																													;
//	virtual		void		BringToDesired						(float time,float velocity,float force=1.f)																			;
	virtual		bool		TryPosition							(Fvector pos,bool exact_state)																										;
	virtual		void		Jump								(const Fvector& jump_velocity)																						;
	virtual		void		SetMaximumVelocity					(dReal vel)																					{m_max_velocity=vel		;}
	virtual		void		InitContact							(dContact* c,bool	&do_collide,u16 material_idx_1,u16 material_idx_2)										;
	virtual		void		SetForcedPhysicsControl				(bool v){m_forced_physics_control=v;}
	virtual		bool		ForcedPhysicsControl				(){return m_forced_physics_control;}
	virtual		void		Create								(dVector3 sizes)																									;
private:
	//virtual		EEnvironment CheckInvironment					();
#ifdef DEBUG
	virtual		void		OnRender							()																													;
#endif

};