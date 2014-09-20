void CPHSimpleCharacter::UpdateStaticDamage(dContact* c,SGameMtl* tri_material,bool bo1)
{
	const	dReal	*v			=	dBodyGetLinearVel(m_body);
			dReal	norm_prg	=	dFabs(dDOT(v,c->geom.normal));
			dReal	smag		=	dDOT(v,v);
			dReal	plane_pgr	=	_sqrt(smag-norm_prg*norm_prg);
			dReal	mag			=	0.f;
				if(tri_material->Flags.test(SGameMtl::flPassable))
				{
					mag					=	_sqrt(smag)*tri_material->fBounceDamageFactor;
				}
				else
				{
					float				vel_prg;vel_prg=_max(plane_pgr*tri_material->fPHFriction,norm_prg);
					mag					=	(vel_prg)*tri_material->fBounceDamageFactor;
				}
				if(mag>m_collision_damage_info.m_contact_velocity)
				{
  					m_collision_damage_info.m_contact_velocity	=	mag;
					m_collision_damage_info.m_dmc_signum		=	bo1 ? 1.f : -1.f;
					m_collision_damage_info.m_dmc_type			=	SCollisionDamageInfo::ctStatic;
					m_collision_damage_info.m_damege_contact	=	*c;
					//m_collision_damage_info.m_object			=	0;
					m_collision_damage_info.m_obj_id				=	u16(-1);
				}
}

void CPHSimpleCharacter::UpdateDynamicDamage(dContact* c,u16 obj_material_idx,dBodyID b,bool bo1)
{
	
	//if(ph_world ->IsFreezed())
							//return;
	const dReal* vel=dBodyGetLinearVel(m_body);
	dReal c_vel;
	dMass m;
	dBodyGetMass(b,&m);

	const dReal* obj_vel=dBodyGetLinearVel(b);
	const dReal* norm=c->geom.normal;
	dReal norm_vel=dDOT(vel,norm);
	dReal norm_obj_vel=dDOT(obj_vel,norm);

	if((bo1&&norm_vel>norm_obj_vel)||
		(!bo1&&norm_obj_vel>norm_vel)
		) return ; 


	dVector3 Pc={vel[0]*m_mass+obj_vel[0]*m.mass,vel[1]*m_mass+obj_vel[1]*m.mass,vel[2]*m_mass+obj_vel[2]*m.mass};
	//dVectorMul(Vc,1.f/(m_mass+m.mass));
	//dVector3 vc_obj={obj_vel[0]-Vc[0],obj_vel[1]-Vc[1],obj_vel[2]-Vc[2]};
	//dVector3 vc_self={vel[0]-Vc[0],vel[1]-Vc[1],vel[2]-Vc[2]};
	//dReal vc_obj_norm=dDOT(vc_obj,norm);
	//dReal vc_self_norm=dDOT(vc_self,norm);

	dReal Kself=norm_vel*norm_vel*m_mass/2.f;
	dReal Kobj=norm_obj_vel*norm_obj_vel*m.mass/2.f;

	dReal Pcnorm=dDOT(Pc,norm);
	dReal KK=Pcnorm*Pcnorm/(m_mass+m.mass)/2.f;
	dReal accepted_energy=Kself*m_collision_damage_factor+Kobj*object_damage_factor-KK;
	//DeltaK=m1*m2*(v1-v2)^2/(2*(m1+m2))
	if(accepted_energy>0.f)
	{
		SGameMtl	*obj_material=GMLib.GetMaterialByIdx(obj_material_idx);
		c_vel=dSqrt(accepted_energy/m_mass*2.f)*obj_material->fBounceDamageFactor;
	}
	else c_vel=0.f;
#ifdef DEBUG
	if(ph_dbg_draw_mask.test(phDbgDispObjCollisionDammage)&&c_vel>dbg_vel_collid_damage_to_display)
	{
		float dbg_my_norm_vell=norm_vel;
		float dbg_obj_norm_vell=norm_obj_vel;
		float dbg_my_kinetic_e=Kself;
		float dbg_obj_kinetic_e=Kobj;
		float dbg_my_effective_e=Kself*m_collision_damage_factor;
		float dbg_obj_effective_e=Kobj*object_damage_factor;
		float dbg_free_energy=KK;
		Msg("-----------------------------------------------------------------------------------------");
		Msg("cd %s -effective vell %f",		*PhysicsRefObject()->cName(),				c_vel);
		Msg("cd %s -my_norm_vell %f",		*PhysicsRefObject()->cName(),				dbg_my_norm_vell);
		Msg("cd %s -obj_norm_vell %f",		*PhysicsRefObject()->cName(),				dbg_obj_norm_vell);
		Msg("cd %s -my_kinetic_e %f",		*PhysicsRefObject()->cName(),				dbg_my_kinetic_e);
		Msg("cd %s -obj_kinetic_e %f",		*PhysicsRefObject()->cName(),				dbg_obj_kinetic_e);
		Msg("cd %s -my_effective_e %f",		*PhysicsRefObject()->cName(),				dbg_my_effective_e);
		Msg("cd %s -obj_effective_e %f",	*PhysicsRefObject()->cName(),				dbg_obj_effective_e);
		Msg("cd %s -effective_acceted_e %f",*PhysicsRefObject()->cName(),				accepted_energy);
		Msg("cd %s -real_acceted_e %f",		*PhysicsRefObject()->cName(),				Kself+Kobj-KK);
		Msg("cd %s -free_energy %f",		*PhysicsRefObject()->cName(),				dbg_free_energy);
		Msg("-----------------------------------------------------------------------------------------");
		/*
		static float dbg_my_norm_vell=0.f;
		static float dbg_obj_norm_vell=0.f;
		static float dbg_my_kinetic_e=0.f;
		static float dbg_obj_kinetic_e=0.f;
		static float dbg_my_effective_e=0.f;
		static float dbg_obj_effective_e=0.f;
		static float dbg_free_energy=0.f;
		if()
			dbg_my_norm_vell=norm_vel;
			dbg_obj_norm_vell=norm_obj_vel;
			dbg_my_kinetic_e=Kself;
			dbg_obj_kinetic_e=Kobj;
			dbg_my_effective_e=Kself*m_collision_damage_factor;
			dbg_obj_effective_e=Kobj*object_damage_factor;
			dbg_free_energy=KK;
		DBG_OutText("-----dbg obj collision damage-------");
		DBG_OutText("my_norm_vell %f",dbg_my_norm_vell);
		DBG_OutText("obj_norm_vell %f",dbg_obj_norm_vell);
		DBG_OutText("my_kinetic_e %f",dbg_my_kinetic_e);
		DBG_OutText("obj_kinetic_e %f", dbg_obj_kinetic_e);
		DBG_OutText("my_effective_e %f",dbg_my_effective_e);
		DBG_OutText("obj_effective_e %f",dbg_obj_effective_e);
		DBG_OutText("free_energy %f",dbg_free_energy);
		DBG_OutText("-----------------------------------");
		*/
	}
#endif
	if(c_vel>m_collision_damage_info.m_contact_velocity) 
	{
		CPhysicsShellHolder* obj=bo1 ? retrieveRefObject(c->geom.g2) : retrieveRefObject(c->geom.g1);
		VERIFY(obj);
		if(!obj->getDestroy())
		{
			m_collision_damage_info.m_contact_velocity=c_vel;
			m_collision_damage_info.m_dmc_signum=bo1 ? 1.f : -1.f;
			m_collision_damage_info.m_dmc_type=SCollisionDamageInfo::ctObject;
			m_collision_damage_info.m_damege_contact=*c;
			m_collision_damage_info.m_hit_callback=obj->get_collision_hit_callback();
			m_collision_damage_info.m_obj_id=obj->ID();
		}
	}
}


IC		void	CPHSimpleCharacter::foot_material_update(u16	contact_material_idx,u16	foot_material_idx)
{
	if(*p_lastMaterialIDX!=u16(-1)&&GMLib.GetMaterialByIdx( *p_lastMaterialIDX)->Flags.test(SGameMtl:: flPassable)&&!b_foot_mtl_check)	return			;
	b_foot_mtl_check					=false									   ;

	const SGameMtl* contact_material = GMLib.GetMaterialByIdx(contact_material_idx);

	if(contact_material->Flags.test(SGameMtl::flPassable))
	{
		if(contact_material->Flags.test(SGameMtl::flInjurious))
			injuriousMaterialIDX = contact_material_idx	;
		else
			*p_lastMaterialIDX	= contact_material_idx	;
								
	}
	else	
								*p_lastMaterialIDX=foot_material_idx					;

}

