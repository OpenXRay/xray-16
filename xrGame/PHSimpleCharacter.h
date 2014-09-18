#pragma once
#include "PHCharacter.h"
#include "Physics.h"
#include "MathUtils.h"
#include "ElevatorState.h"
#include "IColisiondamageInfo.h"
#include "../xrEngine/gamemtllib.h"
namespace ALife {
	enum EHitType;
};
#ifdef DEBUG
#include "PHDebug.h"
#endif


class CPHSimpleCharacter : 
	public CPHCharacter,
	ICollisionDamageInfo
{
	typedef CPHCharacter	inherited;
private:
	
	collide::rq_results			RQR;

protected:
	CElevatorState			m_elevator_state;
	////////////////////////////damage////////////////////////////////////////
#ifdef DEBUG
	public:
#endif
	struct SCollisionDamageInfo
	{
										SCollisionDamageInfo		()										;
				void					Construct					()										;
				float					ContactVelocity				()				const					;
				void					HitDir						(Fvector &dir)	const					;
			IC	const Fvector&			HitPos						()				const					{return cast_fv(m_damege_contact.geom.pos);}
				void					Reinit						()										;
				dContact				m_damege_contact;
				SCollisionHitCallback	*m_hit_callback;
				u16						m_obj_id;
				float					m_dmc_signum;
				enum{ctStatic,ctObject}	m_dmc_type;
				ALife::EHitType			m_hit_type;
		mutable	float					m_contact_velocity;
	};
#ifdef	DEBUG
	SCollisionDamageInfo& dbg_get_collision_dmg_info(){ return m_collision_damage_info ;}
#endif
protected:
	SCollisionDamageInfo		m_collision_damage_info;
	/////////////////////////// callback
	ObjectContactCallbackFun*	m_object_contact_callback;
	////////////////////////// geometry
	Fvector m_last_move;
	dGeomID m_geom_shell;
	dGeomID m_wheel;
	dGeomID m_hat;
	dGeomID	m_cap;

	dGeomID m_hat_transform;
	dGeomID m_wheel_transform;
	dGeomID m_shell_transform;
	dGeomID m_cap_transform;

	dSpaceID m_space;
	
	dReal m_radius;
	dReal m_cyl_hight;
	///////////////////////////////////
	//dJointID m_capture_joint;
	//dJointFeedback m_capture_joint_feedback;
	////////////////////////// movement
	dVector3 m_control_force;
	Fvector	 m_acceleration;	
	Fvector  m_cam_dir;
	dVector3 m_wall_contact_normal;
	dVector3 m_ground_contact_normal;
	dVector3 m_clamb_depart_position;
	dVector3 m_depart_position;
	dVector3 m_wall_contact_position;
	dVector3 m_ground_contact_position;
	dReal	 jump_up_velocity;//=6.0f;//5.6f;
	dReal	 m_collision_damage_factor;
	dReal	 m_max_velocity;

	float   m_air_control_factor;

	dVector3 m_jump_depart_position;
	dVector3 m_death_position;
	Fvector  m_jump_accel;

	Fvector  m_last_environment_update;
	u16		 m_last_picked_material;
	//movement state
	bool is_contact					;
	bool was_contact				;
	bool b_depart					;
	bool b_meet						;
	bool b_side_contact				;
	bool b_was_side_contact			;
	bool b_any_contacts				;
	bool b_air_contact_state		;

	bool b_valide_ground_contact	;
	bool b_valide_wall_contact		;
	bool b_on_object				;
	bool b_was_on_object			;
	bool b_on_ground				;
	bool b_lose_ground				;
	bool b_collision_restrictor_touch;
	u32  m_contact_count			;

	bool	is_control					;
	bool	b_meet_control				;
	bool	b_lose_control				;
	bool	was_control				;
	bool	b_stop_control				;
	bool	b_depart_control			;
	bool	b_jump						;
	bool	b_jumping					;
	bool	b_clamb_jump				;
	bool	b_external_impulse			;
	u64		m_ext_impuls_stop_step		;
	Fvector m_ext_imulse				;
	bool	b_death_pos					;
	bool	b_foot_mtl_check			;
	dReal	m_friction_factor			;
	bool	b_non_interactive			;
public:
							CPHSimpleCharacter					()									;
	virtual					~CPHSimpleCharacter					()						{Destroy();}

	/////////////////CPHObject//////////////////////////////////////////////
	virtual		void		PhDataUpdate						(dReal step)						;
	virtual		void		PhTune								(dReal step)						;
	virtual		void		InitContact							(dContact* c,bool &do_collide,u16 /*material_idx_1*/,u16 /*material_idx_2*/)		;
	virtual		dSpaceID	dSpace								()									{return m_space;}
	virtual		dGeomID		dSpacedGeom							()									{return (dGeomID)m_space;}
	virtual		void		get_spatial_params					()									;
	/////////////////CPHCharacter////////////////////////////////////////////
public:
	//update


	//Check state
	virtual		bool			 		ContactWas						()					{if(b_meet_control) {b_meet_control=false;return true;} else return false;}
	virtual		EEnvironment	 		CheckInvironment				()			;
	virtual		void			 		GroundNormal					(Fvector &norm)		;
	virtual		const ICollisionDamageInfo	*CollisionDamageInfo ()const {return this;}
private:
	virtual		float			 	ContactVelocity				()const				{return m_collision_damage_info.ContactVelocity();}
	virtual		void			 	HitDir							(Fvector& dir)const	{return m_collision_damage_info.HitDir(dir);}
	virtual		const Fvector&	 	HitPos							()const				{return m_collision_damage_info.HitPos();}
	virtual		u16				 	DamageInitiatorID				()const				;
	virtual		CObject			 	*DamageInitiator				()const				;
	virtual		ALife::EHitType	 	HitType							()const				{return m_collision_damage_info.m_hit_type; };
	
	virtual		void				SetHitType						(ALife::EHitType type){ m_collision_damage_info.m_hit_type = type; };
	virtual SCollisionHitCallback	*HitCallback					()const				;
	virtual		void				Reinit							()					{m_collision_damage_info.Reinit();};
public:
	//Creating
	virtual		void		Create								(dVector3 sizes)	;
	virtual		void		Destroy								(void)				;
	virtual		void		Disable								()					;
	virtual		void 		EnableObject						(CPHObject* obj)	;
	virtual		void		Enable								()					;
	virtual		void		SetBox								(const dVector3 &sizes);
	virtual		bool		UpdateRestrictionType				(CPHCharacter* ach);
	//get-set
	virtual		void		SetObjectContactCallback			(ObjectContactCallbackFun* callback);
	virtual		void		SetObjectContactCallbackData		( void* data );
	virtual		void		SetWheelContactCallback				(ObjectContactCallbackFun* callback);
private:
				void		RemoveObjectContactCallback			(ObjectContactCallbackFun* callback);
				void		AddObjectContactCallback			(ObjectContactCallbackFun* callback);
static			void		TestRestrictorContactCallbackFun	(bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2);
public:
	virtual		ObjectContactCallbackFun* ObjectContactCallBack	();
	void					SetStaticContactCallBack			(ContactCallbackFun* calback);
	virtual		void		SwitchOFFInitContact				()					;
	virtual		void		SwitchInInitContact					()					;
	virtual		void		SetAcceleration						(Fvector accel)		;
	virtual		Fvector		GetAcceleration						()					{ return m_acceleration; };
	virtual     void		SetCamDir							(const Fvector& cam_dir);
	virtual	const Fvector&	CamDir								()const				{return m_cam_dir;}
	virtual		void		SetMaterial							(u16 material)		;
	virtual		void		SetPosition							(const Fvector &pos);
	virtual		void		GetVelocity							(Fvector& vvel)const;
	virtual		void		GetSmothedVelocity					(Fvector& vvel)		;
	virtual		void		SetVelocity							(Fvector vel)		;
	virtual		void		SetAirControlFactor					(float factor)		{m_air_control_factor=factor;}
	virtual		void		SetElevator							(CClimableObject* climable){m_elevator_state.SetElevator(climable);};
	virtual	CElevatorState	*ElevatorState						()					;
	virtual		void		SetCollisionDamageFactor			(float f)			{m_collision_damage_factor=f;}
	virtual		void		GetPosition							(Fvector& vpos)	;
	virtual		void		GetPreviousPosition					(Fvector& pos)		;
	virtual		float		FootRadius							()					;
	virtual		void		DeathPosition						(Fvector& deathPos)	;
	virtual		void		IPosition							(Fvector& pos)		;
	virtual		u16			ContactBone							();
	virtual		void		ApplyImpulse						(const Fvector& dir, const dReal P);
	virtual		void		ApplyForce							(const Fvector& force);
	virtual		void		ApplyForce							(const Fvector& dir,float force);
	virtual		void		ApplyForce							(float x,float y, float z);
	virtual		void		AddControlVel						(const Fvector& vel);
	virtual		void		SetMaximumVelocity					(dReal vel)			{m_max_velocity=vel;}
	virtual		dReal		GetMaximumVelocity					()					{ return m_max_velocity;}
	virtual		void		SetJupmUpVelocity					(dReal velocity)	{jump_up_velocity=velocity;}
	virtual		bool		JumpState							()					{
																				return b_jumping||b_jump;
																					};
	virtual	const Fvector&  ControlAccel						()const				{return m_acceleration;}
	virtual		bool		TouchRestrictor						(ERestrictionType rttype);
	virtual		float		&FrictionFactor						()					{return m_friction_factor;}
	virtual		void		SetMas								(dReal mass)		;
	virtual		float		Mass								()					{return m_mass;};
	virtual		void		SetPhysicsRefObject					(CPhysicsShellHolder* ref_object);
	virtual		void		SetNonInteractive					(bool v);	

	//virtual		void		CaptureObject						(dBodyID body,const dReal* anchor);
	//virtual		void		CapturedSetPosition					(const dReal* position);
	//virtual		void		doCaptureExist						(bool&	do_exist);

	virtual		void		get_State							(		SPHNetState&	state)								;
	virtual		void		set_State							(const	SPHNetState&	state)								;
	virtual		void		ValidateWalkOn						();
	bool		ValidateWalkOnMesh					()			;
	bool		ValidateWalkOnObject				()			;
private:
			void		CheckCaptureJoint					();
			void		ApplyAcceleration					();

			u16			RetriveContactBone					();
			void		SafeAndLimitVelocity				();
			void		UpdateStaticDamage					(dContact* c,SGameMtl* tri_material,bool bo1);
			void		UpdateDynamicDamage					(dContact* c,u16 obj_material_idx,dBodyID b,bool bo1);
IC			void 		FootProcess							(dContact* c,bool &do_collide ,bool bo);
IC			void		foot_material_update				(u16	tri_material,u16	foot_material_idx);
			static void	TestPathCallback(bool& do_colide,bool bo1,dContact& c,SGameMtl * /*material_1*/,SGameMtl * /*material_2*/);
virtual		void		Collide								();
			void		OnStartCollidePhase					();				
protected:
virtual	void	get_Box								( Fvector&	sz, Fvector& c )const;
	protected:
virtual	void	update_last_material						();
public:	
#ifdef DEBUG
	virtual		void		OnRender						();
#endif
};

const dReal def_spring_rate=0.5f;
const dReal def_dumping_rate=20.1f;

IC bool ignore_material( u16 material_idx )
{
		SGameMtl* material=GMLib.GetMaterialByIdx( material_idx );
		return !!material->Flags.test( SGameMtl::flActorObstacle );
}