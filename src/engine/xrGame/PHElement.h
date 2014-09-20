/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
///////////////////////////Implemetation//for//CPhysicsElement//////////////////
////////////////////////////////////////////////////////////////////////////////
#include "Geometry.h"
#include "phdefs.h"
#include "PhysicsCommon.h"
#include "PHSynchronize.h"
#include "PHDisabling.h"
#include "PHGeometryOwner.h"
#include "PHInterpolation.h"
#include "PHFracture.h"

#ifndef PH_ELEMENT
#define PH_ELEMENT
class CPHElement;
class CPHShell;


struct SPHImpact;
class CPHFracturesHolder;

class CPHElement	:  
	public	CPhysicsElement ,
	public	CPHSynchronize,
	public	CPHDisablingFull,
	public	CPHGeometryOwner
{
	friend class CPHFracturesHolder;

	//float						m_start_time;				//uu ->to shell ??	//aux
	dMass						m_mass;						//e ??				//bl
	dBodyID						m_body;						//e					//st
	dReal						m_l_scale;					// ->to shell ??	//bl
	dReal						m_w_scale;					// ->to shell ??	//bl
	CPHElement					*m_parent_element;			//bool !			//bl
	CPHShell					*m_shell;					//e					//bl
	CPHInterpolation			m_body_interpolation;		//e					//bl
	CPHFracturesHolder			*m_fratures_holder;			//e					//bl

	dReal						m_w_limit ;					//->to shell ??		//bl
	dReal						m_l_limit ;					//->to shell ??		//bl
//	dVector3					m_safe_position;			//e					//st
//	dQuaternion					m_safe_quaternion;
//	dVector3					m_safe_velocity;			//e					//st
//	Fmatrix						m_inverse_local_transform;	//e				//bt
	dReal						k_w;						//->to shell ??		//st
	dReal						k_l;						//->to shell ??		//st
	//ObjectContactCallbackFun*	temp_for_push_out;			//->to shell ??		//aux
	//u32							push_untill;				//->to shell ??		//st
	Flags8						m_flags;					//
	enum				
	{
		flActive				=	1<<0,
		flActivating			=	1<<1,
		flUpdate				=	1<<2,
		flWasEnabledBeforeFreeze=	1<<3,
		flEnabledOnStep			=	1<<4,
		flFixed					=	1<<5,
		flAnimated				=	1<<6
	};
//	bool						was_enabled_before_freeze;
//	bool						bUpdate;					//->to shell ??		//st
//	bool						b_enabled_onstep;
private:
////////////////////////////////////////////Interpolation/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void					FillInterpolation				()																				//interpolation called anywhere visual influent
	{
		m_body_interpolation.ResetPositions();
		m_body_interpolation.ResetRotations();
		//bUpdate=true;
		m_flags.set(flUpdate,TRUE);
	}
IC	void					UpdateInterpolation				()																				//interpolation called from ph update visual influent
	{
		///VERIFY(dBodyStateValide(m_body));
		m_body_interpolation.UpdatePositions();
		m_body_interpolation.UpdateRotations();
		//bUpdate=true;
		m_flags.set(flUpdate,TRUE);
	}
public:
////////////////////////////////////////////////Geometry/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual	void						add_Sphere								(const Fsphere&		V);															//aux
	virtual	void						add_Box									(const Fobb&		V);															//aux
	virtual	void						add_Cylinder							(const Fcylinder&	V);															//aux
	virtual void						add_Shape								(const SBoneShape& shape);														//aux
	virtual void						add_Shape								(const SBoneShape& shape,const Fmatrix& offset);								//aux
	virtual CODEGeom*					last_geom								(){return CPHGeometryOwner::last_geom();}
	virtual CODEGeom*					geometry								( u16 i ){ return CPHGeometryOwner::Geom( i ); }
	virtual	const IPhysicsGeometry*		geometry								( u16 i )const	{ return CPHGeometryOwner::Geom( i ); };
	virtual	void						add_geom								( CODEGeom* g );
	virtual	void						remove_geom								( CODEGeom* g );
	virtual bool						has_geoms								(){return CPHGeometryOwner::has_geoms();}
	virtual void						set_ContactCallback						(ContactCallbackFun* callback);													//aux (may not be)
	virtual void						set_ObjectContactCallback				(ObjectContactCallbackFun* callback);											//called anywhere ph state influent
	virtual void						add_ObjectContactCallback				(ObjectContactCallbackFun* callback);											//called anywhere ph state influent
	virtual void						remove_ObjectContactCallback			(ObjectContactCallbackFun* callback);
	virtual void						set_CallbackData						(void * cd);
	virtual	void						*get_CallbackData						();
	virtual	ObjectContactCallbackFun	*get_ObjectContactCallback				();
	virtual void						set_PhysicsRefObject					(CPhysicsShellHolder* ref_object);												//aux
	virtual CPhysicsShellHolder*		PhysicsRefObject						(){return m_phys_ref_object;}													//aux
	virtual void						SetMaterial								(u16 m);																		//aux
	virtual void						SetMaterial								(LPCSTR m){CPHGeometryOwner::SetMaterial(m);}									//aux
	virtual u16							numberOfGeoms							()const;																				//aux
	virtual const Fvector&				local_mass_Center						()		{return CPHGeometryOwner::local_mass_Center();}							//aux
	virtual float						getVolume								()		{return CPHGeometryOwner::get_volume();}								//aux
	virtual void						get_Extensions							(const Fvector& axis,float center_prg,float& lo_ext, float& hi_ext) const ;			//aux
	virtual	void						get_MaxAreaDir							(Fvector& dir){CPHGeometryOwner::get_MaxAreaDir(dir);}
	virtual float						getRadius								();
////////////////////////////////////////////////////Mass/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	void								calculate_it_data						(const Fvector& mc,float mass);													//aux
	void								calculate_it_data_use_density			(const Fvector& mc,float density);												//aux
	void								calc_it_fract_data_use_density  		(const Fvector& mc,float density);//sets element mass and fractures parts mass	//aux
	dMass								recursive_mass_summ						(u16 start_geom,FRACTURE_I cur_fracture);										//aux
public:																																				//
	virtual const Fvector&				mass_Center								()const						;														//aux
	virtual void						setDensity								(float M);																		//aux
	virtual float						getDensity								(){return m_mass.mass/m_volume;}												//aux
	virtual void						setMassMC								(float M,const Fvector& mass_center);											//aux
	virtual void						setDensityMC							(float M,const Fvector& mass_center);											//aux
	virtual	void						set_local_mass_center					(const Fvector &mc );
	virtual void						setInertia								(const dMass& M);																//aux
	virtual void						addInertia								(const dMass& M);
	virtual void						add_Mass								(const SBoneShape& shape,const Fmatrix& offset,const Fvector& mass_center,float mass,CPHFracture* fracture=NULL);//aux
	virtual	void						set_BoxMass								(const Fobb& box, float mass);													//aux
	virtual void						setMass									(float M);																		//aux
	virtual float						getMass									(){return m_mass.mass;}															//aux
	virtual	dMass*						getMassTensor							();	//aux
			void						ReAdjustMassPositions					(const Fmatrix &shift_pivot,float density);										//aux
			void						ResetMass								(float density);																//aux
			void						CutVelocity								(float l_limit,float a_limit);
///////////////////////////////////////////////////PushOut///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
private:																																			//
		
public:																																				//
//////////////////////////////////////////////Disable/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						Disable									()	;																			//
	virtual	void						ReEnable								()	;																			//
			void						Enable									()	;																			//aux
	virtual bool						isEnabled								() const	{return isActive()&&dBodyIsEnabled(m_body);}
	virtual	bool						isFullActive							() const	{return isActive()&&!m_flags.test(flActivating);}
	virtual	bool						isActive								() const	{return !!m_flags.test(flActive);}
	virtual void						Freeze									()	;																			//
	virtual void						UnFreeze								()	;																			//
	virtual bool						EnabledStateOnStep						()  {return dBodyIsEnabled(m_body)||m_flags.test(flEnabledOnStep);}							//
////////////////////////////////////////////////Updates///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			bool						AnimToVel								( float dt, float l_limit,float a_limit );
			void						BoneGlPos								(Fmatrix &m, const CBoneInstance* B)const;
			void						ToBonePos								(const CBoneInstance* B);
#ifdef		DEBUG
	virtual void						dbg_draw_velocity						( float scale, u32 color );
	virtual void						dbg_draw_force							( float scale, u32 color );
	virtual void						dbg_draw_geometry						( float scale, u32 color, Flags32 flags = Flags32().assign( 0 ) ) const;
#endif
			void						SetBoneCallbackOverwrite				(bool v);
			void						BonesCallBack							(CBoneInstance* B);																//called from updateCL visual influent
			void						StataticRootBonesCallBack				(CBoneInstance* B);
			void						PhDataUpdate							(dReal step);																	//ph update
			void						PhTune									(dReal step);																	//ph update
	virtual void						Update									();																				//called update CL visual influence
//////////////////////////////////////////////////Dynamics////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						SetAirResistance						(dReal linear=default_k_l, dReal angular=default_k_w)							//aux (may not be)
	{																																				//
		k_w= angular;																																//
		k_l=linear;																																	//
	}																																				//
	virtual	void						GetAirResistance				(float &linear, float &angular)															//
	{																																				//
		linear= k_l;																																//
		angular=k_w;																																//
	}	
	virtual void						applyImpact						(const SPHImpact& impact);																																	//
	virtual void						applyImpulseTrace				(const Fvector& pos, const Fvector& dir, float val,const u16 id)	;					//called anywhere ph state influent
	virtual	void						set_DisableParams				(const SAllDDOParams& params)										;					//
	virtual void						set_DynamicLimits				(float l_limit=default_l_limit,float w_limit=default_w_limit);							//aux (may not be)
	virtual void						set_DynamicScales				(float l_scale=default_l_scale,float w_scale=default_w_scale);							//aux (may not be)
	virtual	void						Fix								();
	virtual	void						SetAnimated						( bool v );
	virtual	void						ReleaseFixed					();
	virtual bool						isFixed							(){return !!(m_flags.test(flFixed));}
	virtual void						applyForce						(const Fvector& dir, float val);															//aux
	virtual void						applyForce						(float x,float y,float z);																//called anywhere ph state influent
	virtual void						applyImpulse					(const Fvector& dir, float val);//aux
	virtual void						applyImpulseVsMC				(const Fvector& pos,const Fvector& dir, float val);										//
	virtual void						applyImpulseVsGF				(const Fvector& pos,const Fvector& dir, float val);										//
	virtual void						applyGravityAccel				(const Fvector& accel);
	virtual void						getForce						(Fvector& force);
	virtual void						getTorque						(Fvector& torque);
	virtual void						get_LinearVel					(Fvector& velocity) const;															//aux
	virtual void						get_AngularVel					(Fvector& velocity)	const;															//aux
	virtual void						set_LinearVel					(const Fvector& velocity);														//called anywhere ph state influent
	virtual void						set_AngularVel					(const Fvector& velocity);														//called anywhere ph state influent
	virtual void						setForce						(const Fvector& force);															//
	virtual void						setTorque						(const Fvector& torque);														//
	virtual void						set_ApplyByGravity				(bool flag)			;															//
	virtual bool						get_ApplyByGravity				()					;															//
///////////////////////////////////////////////////Net////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						get_State						(		SPHNetState&	state);													//
	virtual void						set_State						(const	SPHNetState&	state);													//
	virtual	void						net_Import						(NET_Packet& P)				  ;
	virtual	void						net_Export						(NET_Packet& P)				  ;
///////////////////////////////////////////////////Position///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						SetTransform					(const Fmatrix& m0);															//
	virtual void						TransformPosition				(const Fmatrix &form);
	virtual void						getQuaternion					(Fquaternion& quaternion);														//
	virtual void						setQuaternion					(const Fquaternion& quaternion);												//
	virtual void						SetGlobalPositionDynamic		(const Fvector& position);														//
	virtual void						GetGlobalPositionDynamic		(Fvector* v);																	//
	virtual void						cv2obj_Xfrom					(const Fquaternion& q,const Fvector& pos, Fmatrix& xform);						//
	virtual void						cv2bone_Xfrom					(const Fquaternion& q,const Fvector& pos, Fmatrix& xform);						//
	virtual void						InterpolateGlobalTransform		(Fmatrix* m);																	//called UpdateCL vis influent
	virtual void						InterpolateGlobalPosition		(Fvector* v);																	//aux
	virtual void						GetGlobalTransformDynamic		(Fmatrix* m) const ;																	//aux
IC			void						InverceLocalForm				(Fmatrix&)	;
IC			void						MulB43InverceLocalForm			(Fmatrix&) const;

////////////////////////////////////////////////////Structure/////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual CPhysicsShell*				PhysicsShell					();																				//aux
			CPHShell*					PHShell							();
	virtual void						set_ParentElement				(CPhysicsElement* p){ m_parent_element=(CPHElement*)p; }							//aux
#ifdef	DEBUG
	CPHElement*							parent_element					(){ return m_parent_element; }
#endif
	void								SetShell						(CPHShell* p);																	//aux
	virtual	dBodyID						get_body				()		{return m_body;};																//aux
	virtual	const dBodyID				get_bodyConst			()const	{return m_body;};																//aux
//////////////////////////////////////////////////////Breakable//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	IC CPHFracturesHolder*				FracturesHolder							(){return m_fratures_holder;}											//aux
	IC const CPHFracturesHolder*		constFracturesHolder				()const{return m_fratures_holder;}										//aux
	void								DeleteFracturesHolder					();																		//
	virtual bool						isBreakable								();																		//aux
	virtual u16							setGeomFracturable						(CPHFracture& fracture);												//aux
	virtual CPHFracture&				Fracture								(u16 num);																//aux
			void						SplitProcess							(ELEMENT_PAIR_VECTOR &new_elements);									//aux
			void						PassEndGeoms							(u16 from,u16 to,CPHElement* dest);										//aux
////////////////////////////////////////////////////Build/Activate////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void						Activate				(const Fmatrix& m0, float dt01, const Fmatrix& m2,bool disable=false);					//some isues not to be aux
	virtual void						Activate				(const Fmatrix &transform,const Fvector& lin_vel,const Fvector& ang_vel,bool disable=false);//some isues not to be aux
	virtual void						Activate				(bool disable=false, bool not_set_bone_callbacks = false);									//some isues not to be aux
	virtual void						Activate				(const Fmatrix& start_from, bool disable=false);										//some isues not to be aux
	virtual void						Deactivate				();																						//aux																																			//aux
			void						SetBoneCallback			();
			void						ClearBoneCallback		();
			void						CreateSimulBase			();//create body & cpace																//aux
			void						ReInitDynamics			(const Fmatrix &shift_pivot,float density);												//set body & geom positions					
			void						PresetActive			();																						//
			void						build									();																		//aux
			void						build									(bool disable);															//aux
			void						destroy									();																		//called anywhere ph state influent
			void						Start									();																		//aux
			void						RunSimulation							();																		//called anywhere ph state influent
			void						RunSimulation							(const Fmatrix& start_from);											//
			void						ClearDestroyInfo						();
			void						GetAnimBonePos							(Fmatrix &bp);
	//		bool						CheckBreakConsistent					()
	CPHElement										();																						//aux
	virtual ~CPHElement								();																						//aux
};

IC CPHElement* cast_PHElement(CPhysicsElement* e){return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e));}
IC CPHElement* cast_PHElement(void* e){return static_cast<CPHElement*>(static_cast<CPhysicsElement*>(e));}
IC CPhysicsElement* cast_PhysicsElement(CPHElement* e){return static_cast<CPhysicsElement*>(static_cast<CPHElement*>(e));}
#endif