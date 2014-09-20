///////////////////////////////////////////////////////////////////////

#ifndef PH_SHELL
#define PH_SHELL

class CPHShell;
class CPHShellSplitterHolder;
#include "PHJoint.h"
#include "PHElement.h"
#include "PHDefs.h"
#include "PHShellSplitter.h"
#include "phmovestorage.h"




class CPhysicsShellAnimator;


class CPHShell: public CPhysicsShell,public CPHObject {

	friend class CPHShellSplitterHolder;
	enum				
	{
		flActive									=	1<<0,
		flActivating								=	1<<1,
		flRemoveCharacterCollisionAfterDisable		=	1<<2,
	};
	s16						m_active_count;
	Flags8					m_flags;					
	ELEMENT_STORAGE			elements;
	JOINT_STORAGE			joints;
	CPHShellSplitterHolder	*m_spliter_holder;
	CPHMoveStorage			m_traced_geoms;


	CPhysicsShellAnimator*	m_pPhysicsShellAnimatorC;

private:

#ifdef	DEBUG
	virtual		CPhysicsShellHolder	*ref_object					() { return PhysicsRefObject() ;}
#endif

protected:
	dSpaceID			    m_space;
public:
	Fmatrix					m_object_in_root;
	CPHShell								();							
	virtual ~CPHShell						();
	virtual void			applyImpulseTrace		(const Fvector& pos, const Fvector& dir, float val,const u16 id);
	virtual void			applyHit				(const Fvector& pos, const Fvector& dir, float val,const u16 id,ALife::EHitType hit_type);

	static void 			BonesCallback				(CBoneInstance* B);
	static void 			StataticRootBonesCallBack	(CBoneInstance* B);
	virtual	BoneCallbackFun* GetBonesCallback		()	{return BonesCallback ;}
	virtual BoneCallbackFun* GetStaticObjectBonesCallback()	{ VERIFY( false ); return StataticRootBonesCallBack; }
	virtual	void			add_Element				(CPhysicsElement* E);
	virtual	void			ToAnimBonesPositions	();
	virtual bool			AnimToVelocityState		(float dt, float l_limit, float a_limit );
	virtual void			SetBonesCallbacksOverwrite(bool v);
	void					SetPhObjectInElements	();
	virtual	void			EnableObject			(CPHObject* obj);
	virtual	void			DisableObject			();
	virtual void			SetAirResistance		(dReal linear=default_k_l, dReal angular=default_k_w)
	{
		xr_vector<CPHElement*>::iterator i;
		for(i=elements.begin();elements.end()!=i;++i)
			(*i)->SetAirResistance(linear,angular);
	}
	virtual void			GetAirResistance		(float& linear, float& angular)
	{
		(*elements.begin())->GetAirResistance(linear,angular);
	}
	virtual	void			add_Joint				(CPhysicsJoint* J);

	virtual CPHIsland*		PIsland					(){return &Island();};
	virtual void			applyImpulseTrace		(const Fvector& pos, const Fvector& dir, float val)	;

	virtual void			Update					()	;											

	virtual void			Activate				(const Fmatrix& m0, float dt01, const Fmatrix& m2,bool disable=false);
	virtual void			Activate				(const Fmatrix &transform,const Fvector& lin_vel,const Fvector& ang_vel,bool disable=false);
	virtual void			Activate				(bool disable=false, bool not_set_bone_callbacks = false);
	virtual void			Activate				(const Fmatrix& start_from, bool disable=false){};


	virtual	CPhysicsShellAnimator*	PPhysicsShellAnimator(){return	m_pPhysicsShellAnimatorC;};

private:
			void			activate				(bool disable);	
public:
	virtual	void			Build					(bool disable=false);
	virtual	void			RunSimulation			(bool place_current_forms=true);
	virtual	void			net_Import				(NET_Packet& P);
	virtual	void			net_Export				(NET_Packet& P);
			void			PresetActive			();
			void			AfterSetActive			();
			void			PureActivate			();
	virtual void			Deactivate				();
	virtual const	CGID&	GetCLGroup				()const;
	virtual			void	RegisterToCLGroup		(CGID g)									;
	virtual			bool	IsGroupObject			()											;
	virtual			void	SetIgnoreStatic			()											;
	virtual			void	SetIgnoreDynamic		()											;
	virtual			void	SetRagDoll				()											;
	virtual			void	SetIgnoreRagDoll		()											;

	virtual			void	CreateShellAnimator		( CInifile* ini, LPCSTR section )											;
	virtual			void	SetIgnoreAnimated		()											;

	virtual			void	SetSmall				()											;
	virtual			void	SetIgnoreSmall			()											;
	virtual void			setMass					(float M)									;

	virtual void			setMass1				(float M)									;
	virtual	void			setEquelInertiaForEls	(const dMass& M)							;
	virtual	void			addEquelInertiaToEls	(const dMass& M)							;
	virtual float			getMass					()											;
	virtual void			setDensity				(float M)									;
	virtual float			getDensity				()											;
	virtual float			getVolume				()											;
	virtual	void			get_Extensions			(const Fvector& axis,float center_prg,float& lo_ext, float& hi_ext) const;
	virtual void			applyForce				(const Fvector& dir, float val)				;
	virtual void			applyForce				(float x,float y,float z)					;
	virtual void			applyImpulse			(const Fvector& dir, float val)				;
	virtual void			applyGravityAccel		(const Fvector& accel);
	virtual void			setTorque				(const Fvector& torque);
	virtual void			setForce				(const Fvector& force);
	virtual void			set_JointResistance		(float force)
	{
		JOINT_I i;
		for(i=joints.begin();joints.end() != i;++i)
		{
			(*i)->SetForce(force);
			(*i)->SetVelocity();
		}
		//(*i)->SetForceAndVelocity(force);
	}
	virtual		void				set_DynamicLimits				(float l_limit=default_l_limit,float w_limit=default_w_limit);
	virtual		void				set_DynamicScales				(float l_scale=default_l_scale,float w_scale=default_w_scale);
	virtual		void				set_ContactCallback				(ContactCallbackFun* callback);
	virtual		void				set_ObjectContactCallback		(ObjectContactCallbackFun* callback);
	virtual		void				SetAnimated						( bool v );
	virtual		void				add_ObjectContactCallback		(ObjectContactCallbackFun* callback);
	virtual		void				remove_ObjectContactCallback		(ObjectContactCallbackFun* callback);
	virtual		void				set_CallbackData				(void * cd);
	virtual		void				*get_CallbackData				();
	virtual		void				set_PhysicsRefObject			(CPhysicsShellHolder* ref_object);
				CPhysicsShellHolder*PhysicsRefObject				(){ return (*elements.begin())->PhysicsRefObject(); }
	
	//breakbable interface
	virtual		bool				isBreakable						();
	virtual		bool				isFractured						();
	virtual		CPHShellSplitterHolder*	SplitterHolder				(){return m_spliter_holder;}
	virtual		void				SplitProcess					(PHSHELL_PAIR_VECTOR &out_shels);
	virtual		void				BlockBreaking					(){if(m_spliter_holder)m_spliter_holder->SetUnbreakable();}
	virtual		void				UnblockBreaking					(){if(m_spliter_holder)m_spliter_holder->SetBreakable();}
	virtual		bool				IsBreakingBlocked				(){return m_spliter_holder&&m_spliter_holder->IsUnbreakable();}
	///////	////////////////////////////////////////////////////////////////////////////////////////////
	virtual		void				get_LinearVel					(Fvector& velocity) const ;
	virtual		void				get_AngularVel					(Fvector& velocity) const ;
	virtual		void				set_LinearVel					(const Fvector& velocity);
	virtual		void				set_AngularVel					(const Fvector& velocity);
	virtual		void				TransformPosition				(const Fmatrix &form);
	virtual		void				SetGlTransformDynamic			(const Fmatrix &form);
	virtual		void				set_ApplyByGravity				(bool flag);
	virtual		bool				get_ApplyByGravity				();
	virtual		void				SetMaterial						(u16 m);
	virtual		void				SetMaterial						(LPCSTR m);
	virtual		ELEMENT_STORAGE		&Elements						(){return elements;}
	virtual		CPhysicsElement		*get_Element					(u16 bone_id);
	virtual		CPhysicsElement		*get_Element					(const shared_str & bone_name);
	virtual		CPhysicsElement		*get_Element					(LPCSTR bone_name);
	virtual	const CPhysicsElement	*get_ElementByStoreOrder		(u16 num) const;
	virtual		CPhysicsElement		*get_ElementByStoreOrder		(u16 num);
				CPhysicsElement		*get_PhysicsParrentElement		( u16 bone_id );
	virtual		u16					get_ElementsNumber				()const{return (u16)elements.size();}
	virtual		CPHSynchronize		*get_ElementSync				(u16 element);
	virtual		u16					get_elements_number				(){return get_ElementsNumber();}
	virtual		CPHSynchronize		*get_element_sync				(u16 element){return get_ElementSync(element);}
	virtual		CPhysicsElement		*NearestToPoint					(const Fvector& point, NearestToPointCallback *cb = 0 );
	virtual		CPhysicsJoint		*get_Joint						(u16 bone_id);
	virtual		CPhysicsJoint		*get_Joint						(const shared_str & bone_name);
	virtual		CPhysicsJoint		*get_Joint						(LPCSTR bone_name);
	virtual		CPhysicsJoint		*get_JointByStoreOrder			(u16 num);
	virtual		u16					get_JointsNumber				();
	virtual		CODEGeom			*get_GeomByID					(u16 bone_id);

	virtual		void				Enable							();
	virtual		void				Disable							();
	virtual		void				DisableCollision				();
	virtual		void				EnableCollision					();
	virtual		void				DisableCharacterCollision		();
	virtual		void				SetRemoveCharacterCollLADisable	(){m_flags.set(flRemoveCharacterCollisionAfterDisable,TRUE);}
	virtual		bool				isEnabled						()const {return CPHObject::is_active();}
	virtual		bool				isActive						()const {return !!m_flags.test(flActive);}
	virtual		bool				isFullActive					()const {return isActive()&&!m_flags.test(flActivating);}	
				void				SetNotActivating				(){m_flags.set(flActivating,FALSE);}
//CPHObject	 
	virtual		void				vis_update_activate				();
	virtual		void				vis_update_deactivate	  		();
	virtual		void				PureStep						(float step);
	virtual		void				CollideAll						();
	virtual		void				PhDataUpdate					(dReal step);
	virtual		void				PhTune							(dReal step);
	virtual		void				InitContact						(dContact* c,bool &do_collide,u16 /*material_idx_1*/,u16 /*material_idx_2*/){};
	virtual		void				FreezeContent					();
	virtual		void				UnFreezeContent					();
	virtual		void				Freeze							();
	virtual		void				UnFreeze						();
	virtual		void				NetInterpolationModeON			(){CPHObject::NetInterpolationON();}
	virtual		void				NetInterpolationModeOFF			(){CPHObject::NetInterpolationOFF();}
	virtual		void				StepFrameUpdate					(dReal step){};
	virtual		CPHMoveStorage*		MoveStorage						(){return &m_traced_geoms;}
	virtual		void				build_FromKinematics			(IKinematics* K,BONE_P_MAP* p_geting_map=NULL);
	virtual		void				preBuild_FromKinematics			(IKinematics* K,BONE_P_MAP* p_geting_map);
	virtual		void                ZeroCallbacks					();
	virtual		void				ResetCallbacks					(u16 id,Flags64 &mask);
				void				PlaceBindToElForms				();
	virtual		void				SetCallbacks					( );
	virtual		void				EnabledCallbacks				(BOOL val);
	virtual		void				set_DisableParams				(const SAllDDOParams& params);
	virtual		void				UpdateRoot						();
	virtual		void				SmoothElementsInertia			(float k);
	virtual		void				InterpolateGlobalTransform		(Fmatrix* m);
	virtual		void				InterpolateGlobalPosition		(Fvector* v);
	virtual		void				GetGlobalTransformDynamic		(Fmatrix* m) ;
	virtual		void				GetGlobalPositionDynamic		(Fvector* v);
	virtual		Fmatrix&			ObjectInRoot					(){return m_object_in_root;}
	virtual		void				ObjectToRootForm				(const Fmatrix& form);
	virtual		dSpaceID			dSpace							(){return m_space;}
	virtual		void				SetTransform					(const Fmatrix& m0);

	virtual		void				AddTracedGeom					(u16 element=0,u16 geom=0);
	virtual		void				SetAllGeomTraced				();

	virtual		void				ClearTracedGeoms				();
	virtual		void				DisableGeomTrace				();
	virtual		void				EnableGeomTrace					();

	virtual		void				SetPrefereExactIntegration		();
	virtual		void				CutVelocity						(float l_limit,float a_limit);
///////////	//////////////////////////////////////////////////////////////////////////////////////////
				void				CreateSpace						()																				;
				void				PassEndElements					(u16 from,u16 to,CPHShell *dest)												;
				void				PassEndJoints					(u16 from,u16 to,CPHShell *dest)												;
				void				DeleteElement					(u16 element)																	;
				void				DeleteJoint						(u16 joint)																		;
				u16					BoneIdToRootGeom				(u16 id)																		;
/////////////////////////////////////////////////////////////////////////////////////////////////////
protected:
	virtual		void				get_spatial_params				()																				;
	virtual		dGeomID				dSpacedGeom						()														{return (dGeomID)m_space;}

	virtual		void				ClearRecentlyDeactivated		()																				;
				void				ClearCashedTries				()																				;
private:
				//breakable
				void				setEndElementSplitter	  		()																				;
				void				setElementSplitter		  		(u16 element_number,u16 splitter_position)										;
				void				setEndJointSplitter	  			()																				;
				void				AddSplitter			  			(CPHShellSplitter::EType type,u16 element,u16 joint)							;
				void				AddSplitter			  			(CPHShellSplitter::EType type,u16 element,u16 joint,u16 position)				;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				void				AddElementRecursive				(CPhysicsElement* root_e, u16 id,Fmatrix global_parent,u16 element_number,bool *vis_check)		;
				void				PlaceBindToElFormsRecursive		(Fmatrix parent,u16 id,u16 element,Flags64 &mask);
				void				BonesBindCalculate				(u16 id_from=0);
				void				BonesBindCalculateRecursive		(Fmatrix parent,u16 id);
				void				ZeroCallbacksRecursive			(u16 id)																		;
				void				SetCallbacksRecursive			(u16 id,u16 element)															;
				void				ResetCallbacksRecursive			(u16 id,u16 element,Flags64 &mask)												;
				void				SetJointRootGeom				(CPhysicsElement* root_e,CPhysicsJoint* J)										;
				void				ReanableObject					()																				;
				void				ExplosionHit					(const Fvector& pos, const Fvector& dir, float val,const u16 id)				;
				void				ClearBreakInfo					();
				Fmatrix&			get_animation_root_matrix		( Fmatrix& m );
				void				update_root_transforms			();
IC				CPHElement			&root_element					() { VERIFY( !elements.empty() ); return *(*elements.begin()); }
#ifdef		DEBUG
	virtual		void				dbg_draw_velocity				( float scale, u32 color );
	virtual		void				dbg_draw_force					( float scale, u32 color );
	virtual		void				dbg_draw_geometry				( float scale, u32 color, Flags32 flags = Flags32().assign( 0 ) ) const			;
#endif
};
#endif