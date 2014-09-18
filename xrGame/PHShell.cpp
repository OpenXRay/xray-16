/////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "tri-colliderknoopc/dTriList.h"
#include "PHShellSplitter.h"
#include "PHFracture.h"
#include "PHJointDestroyInfo.h"
#include "SpaceUtils.h"
#include "MathUtils.h"
#include "PhysicsShellHolder.h"
#include "../Include/xrRender/Kinematics.h"
#include "PHCollideValidator.h"
#include "game_object_space.h"
//#pragma warning(disable:4995)
//#pragma warning(disable:4267)
//#include "../3rd party/ode/ode/src/collision_kernel.h"
//#pragma warning(default:4995)
//#pragma warning(default:4267)
///////////////////////////////////////////////////////////////
///#pragma warning(disable:4995)

//#include "../3rd party/ode/ode/src/joint.h"
//#include "../3rd party/ode/ode/src/objects.h"

//#pragma warning(default:4995)
///////////////////////////////////////////////////////////////////

#include "ExtendedGeom.h"
#include "PHElement.h"
#include "PHShell.h"
#include "PHCollideValidator.h"
#include "PHElementInline.h"
#include "PhysicsShellAnimator.h"

#include <boost/noncopyable.hpp>
#ifdef DEBUG
#include	"phdebug.h"
#endif
IC		bool	PhOutOfBoundaries			(const Fvector& v)
{
	return v.y < phBoundaries.y1;
}
CPHShell::~CPHShell				()							
{
	
	m_pKinematics	= 0;
	VERIFY(!isActive());
	
	xr_vector<CPHElement*>::iterator i;
	for(i=elements.begin();elements.end()!=i;++i)
		xr_delete(*i);
	elements.clear();

	xr_vector<CPHJoint*>::iterator j;
	for(j=joints.begin();joints.end()!=j;++j)
		xr_delete(*j);
	joints.clear();
	if(m_spliter_holder)xr_delete(m_spliter_holder);

}
CPHShell::CPHShell()
{
	//bActive=false;
	//bActivating=false;
	m_flags.assign(0);
	m_flags.set(flActivating,FALSE);
	m_flags.set(flActive,FALSE);
	m_space=NULL;
	m_pKinematics=NULL;
	m_spliter_holder=NULL;
	m_object_in_root.identity();
	m_active_count=0;
	m_pPhysicsShellAnimatorC=NULL;
}

void CPHShell::EnableObject(CPHObject* obj)
{
	CPHObject::activate();
	if(m_spliter_holder)m_spliter_holder->Activate();
}
void CPHShell::DisableObject()
{

	CPhysicsShellHolder* ref_object=(*elements.begin())->PhysicsRefObject();
//.	if (!ref_object) return;

	if (ref_object)
		ref_object->on_physics_disable	();

	//InterpolateGlobalTransform(&mXFORM);
	CPHObject::deactivate();
	if(m_spliter_holder)m_spliter_holder->Deactivate();
	if(m_flags.test(flRemoveCharacterCollisionAfterDisable))
										DisableCharacterCollision		();
}
void	 CPHShell::	DisableCharacterCollision		()
{
		CPHCollideValidator::SetCharacterClassNotCollide(*this);
}
void CPHShell::Disable()
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	DisableObject();
	for( ;i!=e;++i)
	{
		(*i)->Disable();
	}
	ClearCashedTries();
}
void CPHShell::DisableCollision()
{
	CPHObject::collision_disable();
}
void CPHShell::EnableCollision()
{
	CPHObject::collision_enable();
}
void CPHShell::ReanableObject()
{
	//if(b_contacts_saved) dJointGroupEmpty(m_saved_contacts);
	//b_contacts_saved=false;

}

void CPHShell::vis_update_activate()
{
	++m_active_count;
	CPhysicsShellHolder* ref_object=(*elements.begin())->PhysicsRefObject();
	if(ref_object&&m_active_count>0)
	{
		m_active_count=0;
		ref_object->processing_activate();
	}
}

void CPHShell::vis_update_deactivate()
{
	--m_active_count;
		//CPhysicsShellHolder* ref_object=(*elements.begin())->PhysicsRefObject();
		//if(ref_object&&!m_flags.test(flProcessigDeactivated))
		//{
		//	//ref_object->processing_deactivate();
		//	m_flags.set(flProcessigDeactivate,TRUE);
		//}
}
void CPHShell::setDensity(float M)
{
	ELEMENT_I i;
	//float volume=0.f;
	//for(i=elements.begin();elements.end() != i;++i)	volume+=(*i)->get_volume();

	for(i=elements.begin();elements.end() != i;++i)
		(*i)->setDensity(M);
}


void CPHShell::setMass(float M){
	ELEMENT_I i;
	float volume=0.f;
	for(i=elements.begin();elements.end() != i;++i)	volume+=(*i)->get_volume();

	for(i=elements.begin();elements.end() != i;++i)
		(*i)->setMass(
		(*i)->get_volume()/volume*M
		);
}

void CPHShell::setMass1(float M){
	ELEMENT_I i;


	for(i=elements.begin();elements.end() != i;++i)
		(*i)->setMass(
		M/elements.size()
		);
}
float CPHShell::getMass()
{
	float m=0.f;

	ELEMENT_I i;

	for(i=elements.begin();elements.end() != i;++i)	m+=(*i)->getMass();

	return m;
}

void  CPHShell::get_spatial_params()
{
	spatialParsFromDGeom((dGeomID)m_space,spatial.sphere.P,AABB,spatial.sphere.R);
}

float CPHShell::getVolume()
{
	float v=0.f;

	ELEMENT_I i;

	for(i=elements.begin();elements.end() != i;++i)	v+=(*i)->getVolume();

	return v;
}

float	CPHShell::getDensity()
{
	return getMass()/getVolume();
}




void CPHShell::PhDataUpdate(dReal step){

	ELEMENT_I i=elements.begin(),e=elements.end();
	bool disable=true;
	for(; e!=i ;++i)
	{
		(*i)->PhDataUpdate(step);
		dBodyID body=(*i)->get_body();
		if(body&&disable&&dBodyIsEnabled(body))
											disable=false;
	}
	if(disable) 
	{
				DisableObject();
				CPHObject::put_in_recently_deactivated();
	}
	else		
		ReanableObject();
#if 0
	DBG_OpenCashedDraw();
	dbg_draw_velocity	( 0.1f, D3DCOLOR_XRGB( 255, 0, 0 ) ); 
	dbg_draw_force		( 0.1f, D3DCOLOR_XRGB( 0, 0, 255 ) ); 
	DBG_ClosedCashedDraw( 10000 );
	//dbg_draw_geometry
#endif
	if(PhOutOfBoundaries(cast_fv(dBodyGetPosition((*elements.begin())->get_body()))))
								Disable();

}



void CPHShell::PhTune(dReal step){
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(; e!=i ;++i)
		(*i)->PhTune(step);
}

void CPHShell::Update(){
	if(!isActive()) return;
	if(m_flags.test(flActivating)) m_flags.set(flActivating,FALSE);
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->Update();

	mXFORM.set((*elements.begin())->mXFORM);
	VERIFY2(_valid(mXFORM),"invalid position in update");
}

void	CPHShell::Freeze()
{
	CPHObject::Freeze();
}
void	CPHShell::UnFreeze()
{
	CPHObject::UnFreeze();
}
void	CPHShell::FreezeContent()
{
	
	CPHObject::FreezeContent();
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(; e!=i ;++i)
		(*i)->Freeze();
	
}
void	CPHShell::UnFreezeContent()
{
	CPHObject::UnFreezeContent();
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(; e!=i ;++i)
		(*i)->UnFreeze();
	
}
void		CPHShell::	applyForce				(const Fvector& dir, float val)				
{
	if(!isActive()) return;
	ELEMENT_I i=elements.begin(),e=elements.end();
	val/=getMass();
	for(; e!=i ;++i)
		(*i)->applyForce( dir, val*(*i)->getMass());
	EnableObject(0);
};
void		CPHShell::	applyForce				(float x,float y,float z)				
{
Fvector dir;dir.set(x,y,z);
float val=dir.magnitude();
	if(!fis_zero(val))
	{
		dir.mul(1.f/val);
		applyForce(dir,val);
	}
};
void	CPHShell::		applyImpulse			(const Fvector& dir, float val)				
{
	if(!isActive()) return;
	(*elements.begin())->applyImpulse			( dir, val);
	EnableObject(0);
};
void	CPHShell::	applyImpulseTrace		(const Fvector& pos, const Fvector& dir, float val){
	if(!isActive()) return;
	(*elements.begin())->applyImpulseTrace		( pos,  dir,  val, 0);
	EnableObject(0);
}

void	CPHShell::	applyImpulseTrace		(const Fvector& pos, const Fvector& dir, float val,const u16 id){
	if(!isActive()) return;
	VERIFY(m_pKinematics);
	CBoneInstance& instance=m_pKinematics->LL_GetBoneInstance				(id);
	if(instance.callback_type()!=bctPhysics || !instance.callback_param()) return;

	((CPhysicsElement*)instance.callback_param())->applyImpulseTrace		( pos,  dir,  val, id);
	EnableObject(0);
}

CPhysicsElement* CPHShell::get_Element		(const shared_str & bone_name)
{
	VERIFY(m_pKinematics);
	return get_Element(m_pKinematics->LL_BoneID(bone_name));
}
CPhysicsElement* CPHShell::get_Element		(LPCSTR bone_name)
{
		return get_Element((const shared_str&)(bone_name));
}
CPhysicsElement* CPHShell::get_ElementByStoreOrder(u16 num)
{
	R_ASSERT2(num<elements.size(),"argument is out of range");
	return cast_PhysicsElement(elements[num]);
}
const CPhysicsElement *CPHShell::get_ElementByStoreOrder(u16 num) const
{
	R_ASSERT2(num<elements.size(),"argument is out of range");
	return cast_PhysicsElement(elements[num]);
}
CPHSynchronize*	CPHShell::get_ElementSync			  ( u16 element )
{
	return smart_cast<CPHSynchronize*>(elements[element]);
}

CPhysicsElement* CPHShell::get_PhysicsParrentElement( u16 bone_id )
{
	VERIFY( PKinematics() );
	CPhysicsElement* E		= get_Element( bone_id );
	u16 bid					= bone_id;
	while( !E && bid !=  PKinematics()->LL_GetBoneRoot() )
	{
		CBoneData	&bd =	PKinematics()->LL_GetData( bid );
					bid =	bd.GetParentID();
		E =			get_Element( bid );
	}
	return E;
}

CPhysicsElement* CPHShell::get_Element( u16 bone_id )
{
	if(m_pKinematics&& isActive())
	{
		CBoneInstance& instance=m_pKinematics->LL_GetBoneInstance				(bone_id);
		if(instance.callback()==BonesCallback||instance.callback()==StataticRootBonesCallBack)
		{
			return (instance.callback_type()==bctPhysics)?(CPhysicsElement*)instance.callback_param():NULL;
		}
	}

	ELEMENT_I i=elements.begin(),e=elements.end();
	for(; e!=i ;++i)
		if((*i)->m_SelfID==bone_id)
			return (CPhysicsElement*)(*i);
	return NULL;
}

CPhysicsJoint* CPHShell::get_Joint(u16 bone_id)
{
	JOINT_I i= joints.begin(),e=joints.end();
	for(;e!=i;i++)
		if((*i)->BoneID()==bone_id)
			return (CPhysicsJoint*)(*i);
	return NULL;
}
CPhysicsJoint* CPHShell::get_Joint(const shared_str &bone_name)
{
	VERIFY(m_pKinematics);
	return get_Joint(m_pKinematics->LL_BoneID(bone_name));
}

CPhysicsJoint* CPHShell::get_Joint(LPCSTR bone_name)
{
	return get_Joint((const shared_str&)bone_name);
}

CPhysicsJoint* CPHShell::get_JointByStoreOrder		(u16 num)
{
	return (CPhysicsJoint*) joints[num];
}

u16			CPHShell::get_JointsNumber				()
{
	return u16(joints.size());
}

void	CPHShell:: update_root_transforms			()
{
	u16 anim_root = PKinematics()->LL_GetBoneRoot();
	u16 phys_root = root_element().m_SelfID;
	VERIFY( BI_NONE != anim_root );
	VERIFY( BI_NONE != phys_root );

	if( anim_root == phys_root )
	{
		mXFORM.set( root_element().mXFORM );
		return;
	}
	

	//Fmatrix anim_root_transform = PKinematics()->LL_GetBindTransform( anim_root );


}

void  CPHShell:: BonesCallback				( CBoneInstance* B ){
	///CPHElement*	E			= smart_cast<CPHElement*>	(static_cast<CPhysicsBase*>(B->Callback_Param));

	CPHElement*	E	= cast_PHElement( B->callback_param() );
	//if( E == &root_element() )
	//{
		
	//}
	E->BonesCallBack(B);
	R_ASSERT2( _valid(B->mTransform), "CPHShell:: BonesCallback" );
}


void  CPHShell::StataticRootBonesCallBack			(CBoneInstance* B){
	///CPHElement*	E			= smart_cast<CPHElement*>	(static_cast<CPhysicsBase*>(B->Callback_Param));

	CPHElement*	E			= cast_PHElement(B->callback_param());
	E->StataticRootBonesCallBack(B);
}


void CPHShell::SetTransform	(const Fmatrix& m0){

	mXFORM.set(m0);
	ELEMENT_I i=elements.begin();
	for( ;elements.end() != i; ++i)
	{
		(*i)->SetTransform(m0);
	}
	spatial_move();
}


void CPHShell::Enable()
{
	if(!isActive())
		return;

	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	//if(dBodyIsEnabled((*i)->get_body())) return;
	for( ;i!=e;++i)
		(*i)->Enable();
	EnableObject(0);
}

void CPHShell::set_PhysicsRefObject	 (CPhysicsShellHolder* ref_object)
{
	

 	if(elements.empty()) return;
	if((*elements.begin())->PhysicsRefObject()==ref_object) return;
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
	{
		(*i)->set_PhysicsRefObject(ref_object);
	}



}



void CPHShell::set_ContactCallback(ContactCallbackFun* callback)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->set_ContactCallback(callback);
}


void CPHShell::set_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->set_ObjectContactCallback(callback);
}
void CPHShell::add_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->add_ObjectContactCallback(callback);
}
void CPHShell::remove_ObjectContactCallback(ObjectContactCallbackFun* callback)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->remove_ObjectContactCallback(callback);
}
void CPHShell::set_CallbackData(void *cd)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
		(*i)->set_CallbackData(cd);
}
void CPHShell::SetPhObjectInElements()
{
	if(!isActive()) return;
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i )
		(*i)->SetPhObjectInGeomData((CPHObject*)this);
}

void CPHShell::SetMaterial(LPCSTR m)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
	{
		(*i)->SetMaterial(m);
	}
}

void CPHShell::SetMaterial(u16 m)
{
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
	{
		(*i)->SetMaterial(m);
	}
}

void CPHShell::get_LinearVel(Fvector& velocity) const
{

	(*elements.begin())->get_LinearVel(velocity);
}

void CPHShell::get_AngularVel(Fvector& velocity) const
{

	(*elements.begin())->get_AngularVel(velocity);
}

void	CPHShell::set_LinearVel(const Fvector& velocity)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;i++) (*i)->set_LinearVel(velocity);
}

void	CPHShell::set_AngularVel(const Fvector& velocity)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;i++) (*i)->set_AngularVel(velocity);
}


void CPHShell::TransformPosition(const Fmatrix &form)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;i++) (*i)->TransformPosition(form);
}

void CPHShell::SetGlTransformDynamic(const Fmatrix &form)
{
	VERIFY(isActive());
	VERIFY(_valid(form));
	Fmatrix current,replace;
	GetGlobalTransformDynamic(&current);
	current.invert();
	replace.mul(form,current);
	TransformPosition(replace);
}
void CPHShell::SmoothElementsInertia(float k)
{
	dMass m_avrg;
	dReal krc=1.f-k;
	dMassSetZero(&m_avrg);
	ELEMENT_I i;
	for(i=elements.begin();elements.end() != i;++i)
	{

		dMassAdd(&m_avrg,(*i)->getMassTensor());

	}
	int n = (int)elements.size();
	m_avrg.mass*=k/float(n);
	for(int j=0;j<4*3;++j) m_avrg.I[j]*=k/float(n);

	for(i=elements.begin();elements.end() != i;++i)
	{
		dVector3 tmp;
		dMass* m=(*i)->getMassTensor();
		dVectorSet(tmp,m->c);

		m->mass*=krc;
		for(int j=0;j<4*3;++j) m->I[j]*=krc;
		dMassAdd(m,&m_avrg);

		dVectorSet(m->c,tmp);
	}
}

void CPHShell::setEquelInertiaForEls(const dMass& M)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		(*i)->setInertia(M);
	}
}

void CPHShell::addEquelInertiaToEls(const dMass& M)
{
	ELEMENT_I i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		(*i)->addInertia(M);
	}
}
static BONE_P_MAP* spGetingMap=NULL;
void CPHShell::build_FromKinematics(IKinematics* K,BONE_P_MAP* p_geting_map)
{
	VERIFY( K );
	phys_shell_verify_model ( *K );
	m_pKinematics			=K;
	spGetingMap				=p_geting_map;
	//CBoneData& bone_data	= m_pKinematics->LL_GetData(0);
	if(!m_spliter_holder) m_spliter_holder=xr_new<CPHShellSplitterHolder>(this);
	bool vis_check = false;
	AddElementRecursive(0,m_pKinematics->LL_GetBoneRoot(),Fidentity,0,&vis_check);
	//R_ASSERT2((*elements.begin())->numberOfGeoms(),"No physics shapes was assigned for model or no shapes in main root bone!!!");
	//SetCallbacks(BonesCallback);
	if(m_spliter_holder->isEmpty())ClearBreakInfo();
}

void CPHShell::preBuild_FromKinematics(IKinematics* K,BONE_P_MAP* p_geting_map)
{
	VERIFY( K );
	phys_shell_verify_model ( *K );
	m_pKinematics			=K;
	spGetingMap				=p_geting_map;
	//CBoneData& bone_data	= m_pKinematics->LL_GetData(0);
	if(!m_spliter_holder) m_spliter_holder=xr_new<CPHShellSplitterHolder>(this);
	bool vis_check=false;
	AddElementRecursive(0,m_pKinematics->LL_GetBoneRoot(),Fidentity,0,&vis_check);
	//R_ASSERT2((*elements.begin())->numberOfGeoms(),"No physics shapes was assigned for model or no shapes in main root bone!!!");
	if(m_spliter_holder->isEmpty())ClearBreakInfo();
	m_pKinematics=0;
}
void CPHShell::ClearBreakInfo()
{
	{		
		ELEMENT_I i=elements.begin(),e=elements.end();
		for(;i!=e;++i)(*i)->ClearDestroyInfo();
	}

	{
		JOINT_I i=joints.begin(),e=joints.end();
		for(;i!=e;++i) (*i)->ClearDestroyInfo();
	}	
	xr_delete(m_spliter_holder);
}

	
ICF bool no_physics_shape(const SBoneShape& shape)
{
	return shape.type==SBoneShape::stNone||shape.flags.test(SBoneShape::sfNoPhysics);
}

bool shape_is_physic( const SBoneShape& shape )
{
	return !no_physics_shape( shape );
}

void CPHShell::AddElementRecursive(CPhysicsElement* root_e, u16 id,Fmatrix global_parent,u16 element_number,bool* vis_check)
{

	//CBoneInstance& B	= m_pKinematics->LL_GetBoneInstance(u16(id));
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));
	SJointIKData& joint_data=bone_data.IK_data;
	Fmatrix fm_position;
	fm_position.set		(bone_data.bind_transform);
	fm_position.mulA_43	(global_parent);
	Flags64 mask;
	mask.assign(m_pKinematics->LL_GetBonesVisible());
	bool no_visible=!mask.is(1ui64<<(u64)id);
	bool lvis_check=false;
	if(no_visible)
	{
	
		for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
			AddElementRecursive		(root_e,(*it)->GetSelfID(),fm_position,element_number,&lvis_check);
		return;
	}
	
	CPhysicsElement* E  = 0;
	CPhysicsJoint*   J	= 0;
	bool	breakable=joint_data.ik_flags.test(SJointIKData::flBreakable)	&& 
		root_e																&&
		!(
		no_physics_shape(bone_data.shape)&&
		joint_data.type==jtRigid
		)				
		;

	///////////////////////////////////////////////////////////////
	lvis_check=(check_obb_sise(bone_data.obb));
	
	bool *arg_check=vis_check;
	if(breakable||!root_e)//.
	{
		arg_check=&lvis_check;
	}
	else
	{
		*vis_check=*vis_check||lvis_check;
	}
	/////////////////////////////////////////////////////////////////////

	bool element_added=false;//set true when if elemen created and added by this call
	u16	 splitter_position=0;
	u16 fracture_num=u16(-1);

	if(!no_physics_shape(bone_data.shape) || !root_e)	//
	{	

		if(joint_data.type==jtRigid && root_e ) //
		{

			Fmatrix vs_root_position;
			vs_root_position.set(root_e->mXFORM);
			vs_root_position.invert();
			vs_root_position.mulB_43(fm_position);

			E=root_e;
			if(breakable)
			{
				CPHFracture fracture;
				fracture.m_bone_id					=id;
				R_ASSERT2(id<64,"ower 64 bones in breacable are not supported");
				fracture.m_start_geom_num			=E->numberOfGeoms();
				fracture.m_end_geom_num				=u16(-1);
				fracture.m_start_el_num				=u16(elements.size());
				fracture.m_start_jt_num				=u16(joints.size());	 
				fracture.MassSetFirst				(*(E->getMassTensor()));
				fracture.m_pos_in_element			.set(vs_root_position.c);
				VERIFY								(u16(-1)!=fracture.m_start_geom_num);
				fracture.m_break_force				=joint_data.break_force;
				fracture.m_break_torque				=joint_data.break_torque;
				root_e->add_Shape(bone_data.shape,vs_root_position);
				root_e->add_Mass(bone_data.shape,vs_root_position,bone_data.center_of_mass,bone_data.mass,&fracture);

				fracture_num=E->setGeomFracturable(fracture);
			}
			else
			{
				root_e->add_Shape(bone_data.shape,vs_root_position);
				root_e->add_Mass(bone_data.shape,vs_root_position,bone_data.center_of_mass,bone_data.mass);
			}


			//B.Callback_Param=root_e;
			//B.Callback=NULL;

		}
		else										//
		{
			E	= P_create_Element();
			E->m_SelfID=id;
			E->mXFORM.set		(fm_position);
			E->SetMaterial		(bone_data.game_mtl_idx);
			//Fvector mc;
			//fm_position.transform_tiny(mc,bone_data.center_of_mass);
			E->set_ParentElement(root_e);
			///B.set_callback(BonesCallback1,E);
			if(!no_physics_shape(bone_data.shape)){
				E->add_Shape(bone_data.shape);
				E->setMassMC(bone_data.mass,bone_data.center_of_mass);
			}
			element_number=u16(elements.size());
			add_Element(E);
			element_added=true;


			if(root_e)
			{
				switch(joint_data.type) 
				{
				case jtSlider: 
					{
						J= P_create_Joint(CPhysicsJoint::slider,root_e,E);
						J->SetAnchorVsSecondElement	(0,0,0);
						J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
						J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);
						J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);
						if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
						{
							J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,1);
							J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,1);
						}
						break;
					}
				case jtCloth: 
					{
						J= P_create_Joint(CPhysicsJoint::ball,root_e,E);
						J->SetAnchorVsSecondElement	(0,0,0);
						J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
						break;
					}
				case jtJoint:
					{
						bool	eqx=!!fsimilar(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y),
							eqy=!!fsimilar(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y),
							eqz=!!fsimilar(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y);

						if(eqx)
						{
							if(eqy)
							{
								J= P_create_Joint(CPhysicsJoint::hinge,root_e,E);
								J->SetAnchorVsSecondElement	(0,0,0);
								J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
								J->SetAxisDirVsSecondElement (0.f,0.f,1.f,0);
								if(joint_data.limits[2].limit.y-joint_data.limits[2].limit.x<M_PI*2.f)
								{
									J->SetLimits(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y,0);
									J->SetAxisSDfactors(joint_data.limits[2].spring_factor,joint_data.limits[2].damping_factor,0);
								}
								break;
							}
							if(eqz)
							{
								J= P_create_Joint(CPhysicsJoint::hinge,root_e,E);
								J->SetAnchorVsSecondElement	(0,0,0);
								J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
								J->SetAxisDirVsSecondElement(0,1,0,0);
								if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
								{
									J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,0);
									J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,0);
								}
								break;
							}
							J= P_create_Joint(CPhysicsJoint::full_control,root_e,E);
							J->SetAnchorVsSecondElement	(0,0,0);
							J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
							J->SetAxisDirVsSecondElement(0.f,0.f,1.f,0);//2-0
							//0-1
							J->SetAxisDirVsSecondElement(0.f,1.f,0.f,2);//1-2

							if(joint_data.limits[2].limit.y-joint_data.limits[2].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y,0);
								J->SetAxisSDfactors(joint_data.limits[2].spring_factor,joint_data.limits[2].damping_factor,0);
							}
							if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,1);
								J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,1);
							}

							if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,2);
								J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,2);
							}

							break;

						}

						if(eqy)
						{
							if(eqz)
							{
								J= P_create_Joint(CPhysicsJoint::hinge,root_e,E);
								J->SetAnchorVsSecondElement	(0,0,0);
								J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
								J->SetAxisDirVsSecondElement(1,0,0,0);
								if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
								{
									J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);
									J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);
								}
								break;
							}

							J= P_create_Joint(CPhysicsJoint::full_control,root_e,E);
							J->SetAnchorVsSecondElement	(0,0,0);
							J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
							J->SetAxisDirVsSecondElement(0.f,0.f,1.f,0);//2-0
							//1-1
							J->SetAxisDirVsSecondElement(1.f,0.f,0.f,2);//0-2
							if(joint_data.limits[2].limit.y-joint_data.limits[2].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y,0);
								J->SetAxisSDfactors(joint_data.limits[2].spring_factor,joint_data.limits[2].damping_factor,0);
							}
							if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,2);
								J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,2);
							}

							if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,1);
								J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,1);
							}
							break;
						}

						if(eqz)
						{
							J= P_create_Joint(CPhysicsJoint::full_control,root_e,E);
							J->SetAnchorVsSecondElement	(0,0,0);
							J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
							J->SetAxisDirVsSecondElement(1.f,0.f,0.f,0);//0-0
							//2-1
							J->SetAxisDirVsSecondElement(0.f,1.f,0.f,2);//1-2
							if(joint_data.limits[2].limit.y-joint_data.limits[2].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y,1);
								J->SetAxisSDfactors(joint_data.limits[2].spring_factor,joint_data.limits[2].damping_factor,1);
							}
							if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);
								J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);
							}

							if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
							{
								J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,2);
								J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,2);
							}
							break;
						}
						J= P_create_Joint(CPhysicsJoint::full_control,root_e,E);
						J->SetAnchorVsSecondElement	(0,0,0);
						J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
						J->SetAxisDirVsSecondElement(0.f,0.f,1.f,0);//2-0
						//0-1
						J->SetAxisDirVsSecondElement(0.f,1.f,0.f,2);//1-2
						if(joint_data.limits[2].limit.y-joint_data.limits[2].limit.x<M_PI*2.f)
						{
							J->SetLimits(joint_data.limits[2].limit.x,joint_data.limits[2].limit.y,0);
							J->SetAxisSDfactors(joint_data.limits[2].spring_factor,joint_data.limits[2].damping_factor,0);
						}
						if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
						{
							J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,1);
							J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,1);
						}

						if(joint_data.limits[1].limit.y-joint_data.limits[1].limit.x<M_PI*2.f)
						{
							J->SetLimits(joint_data.limits[1].limit.x,joint_data.limits[1].limit.y,2);
							J->SetAxisSDfactors(joint_data.limits[1].spring_factor,joint_data.limits[1].damping_factor,2);
						}

						break;
					}
				case jtWheel:
					{
						J= P_create_Joint(CPhysicsJoint::hinge2,root_e,E);
						J->SetAnchorVsSecondElement	(0,0,0);
						J->SetJointSDfactors(joint_data.spring_factor,joint_data.damping_factor);
						J->SetAxisDirVsSecondElement(1,0,0,0);
						J->SetAxisDirVsSecondElement(0,0,1,1);
						if(joint_data.limits[0].limit.y-joint_data.limits[0].limit.x<M_PI*2.f)
						{
							J->SetLimits(joint_data.limits[0].limit.x,joint_data.limits[0].limit.y,0);	
							J->SetAxisSDfactors(joint_data.limits[0].spring_factor,joint_data.limits[0].damping_factor,0);
						}
						break;
					}

				case jtNone: break;

				default: NODEFAULT;
				}
				if(J)
				{
					
					J->SetForceAndVelocity(joint_data.friction);//joint_data.friction
					SetJointRootGeom(root_e,J);
					J->SetBoneID(id);
					add_Joint	(J);
					if(breakable)
					{
						setEndJointSplitter();
						J->SetBreakable(joint_data.break_force,joint_data.break_torque);
					}
				}
			}
			if(m_spliter_holder)
			{
				splitter_position=u16(m_spliter_holder->m_splitters.size());
			}
		}	
	}
	else
	{	
		//B.set_callback(0,root_e);
		E=root_e;
	}
	
	if(!no_physics_shape(bone_data.shape))
	{
		CODEGeom* added_geom	=	E->last_geom();
		if(added_geom)
		{
			added_geom->set_bone_id(id);
			added_geom->set_shape_flags( bone_data.shape.flags );
		}
	}
#ifdef DEBUG
	if(E->last_geom())
	VERIFY(E->last_geom()->bone_id()!=u16(-1));
#endif
	if(m_spliter_holder&&E->has_geoms())
	{
		m_spliter_holder->AddToGeomMap(mk_pair(id,E->last_geom())); 
	}

	if(spGetingMap)
	{
		const BONE_P_PAIR_IT c_iter= spGetingMap->find(id);
		if(spGetingMap->end()!=c_iter)
		{
			c_iter->second.joint=J;
			c_iter->second.element=E;
		}
	}


	/////////////////////////////////////////////////////////////////////////////////////
	for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
		AddElementRecursive		(E,(*it)->GetSelfID(),fm_position,element_number,arg_check);
	/////////////////////////////////////////////////////////////////////////////////////
	if(breakable)
	{
		if(joint_data.type==jtRigid)
		{
			CPHFracture& fracture=E->Fracture(fracture_num);
			fracture.m_bone_id			=id;
			fracture.m_end_geom_num		=E->numberOfGeoms();
			fracture.m_end_el_num		=u16(elements.size());//just after this el = current+1
			fracture.m_end_jt_num		=u16(joints.size());	 //current+1

		}
		else
		{
			if(J)
			{
				J->JointDestroyInfo()->m_end_element=u16(elements.size());
				J->JointDestroyInfo()->m_end_joint=u16(joints.size());
			}
		}
	}

	if(element_added&&E->isBreakable())setElementSplitter(element_number,splitter_position);
#ifdef DEBUG
	bool bbb = lvis_check||(!breakable && root_e);
	if(!bbb)
	{	
		IKinematics* K = m_pKinematics;

		Msg("all bones transform:--------");
		
		for(u16 ii=0; ii<K->LL_BoneCount();++ii){
			Fmatrix tr;

			tr = K->LL_GetTransform(ii);
			Log("bone ",K->LL_BoneName_dbg(ii));
			Log("bone_matrix",tr);
		}
		Log("end-------");
	}

	VERIFY3(bbb,*dbg_obj->cNameVisual(),"has breaking parts with no vertexes or size less than 1mm");//
#endif
	
}

void CPHShell::ResetCallbacks(u16 id,Flags64 &mask)
{
	ResetCallbacksRecursive(id,u16(-1),mask);
}

void CPHShell::ResetCallbacksRecursive(u16 id,u16 element,Flags64 &mask)
{

	//if(elements.size()==element)	return;
	CBoneInstance& B	= m_pKinematics->LL_GetBoneInstance(u16(id));
	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));
	SJointIKData& joint_data=bone_data.IK_data;

	if( mask.is(1ui64<<(u64)id) )
	{

		if(no_physics_shape(bone_data.shape)||joint_data.type==jtRigid&& element!=u16(-1))
		{

			B.set_callback(bctPhysics,0,cast_PhysicsElement(elements[element]));
		}
		else
		{

			element++;
			R_ASSERT2(element<elements.size(),"Out of elements!!");
			//if(elements.size()==element)	return;
			B.set_callback(bctPhysics,BonesCallback,cast_PhysicsElement(elements[element]));
			B.set_callback_overwrite(TRUE);
		}
	}
	for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
		ResetCallbacksRecursive((*it)->GetSelfID(),element,mask);
}

void CPHShell::EnabledCallbacks(BOOL val)
{
	if (val){	
		SetCallbacks( );
		// set callback owervrite in used bones
		ELEMENT_I i,e;
		i=elements.begin(); e=elements.end();
		for( ;i!=e;++i){
			CBoneInstance& B	= m_pKinematics->LL_GetBoneInstance((*i)->m_SelfID);
			B.set_callback_overwrite(TRUE);
		}
	}else		ZeroCallbacks();
}



template< typename T>
void for_each_bone_id( IKinematics &K, T op )
{
	u16 bn =  K.LL_BoneCount();
	for(u16 i = 0; i < bn; ++i )
		op( i );
}

CPHElement* get_physics_parent( IKinematics &k,  u16 id )
{
	VERIFY( BI_NONE != id );
	
	while( true )
	{
		CBoneInstance	& B			= k.LL_GetBoneInstance( u16(id) );
		CBoneData		& bone_data	= k.LL_GetData( u16(id) );
		if( B.callback_type() == bctPhysics && B.callback_param() )
			return cast_PHElement( B.callback_param() );

		if( k.LL_GetBoneRoot() == id )
			return 0;

		id = bone_data.GetParentID();

		if( BI_NONE == id )
			return 0;
	}
}
static u16 element_position_in_set_calbacks=u16(-1);
void CPHShell::SetCallbacks( )
{
	
	struct set_bone_callback
	{
		void operator() ( CPHElement* e){e->SetBoneCallback(); }
	};
	std::for_each( elements.begin(), elements.end(), set_bone_callback() );

	struct set_bone_reference: private boost::noncopyable
	{
		IKinematics &K;
		set_bone_reference( IKinematics &K_ ): K( K_ ){}
		void operator() ( u16 id )
		{
			CBoneInstance &bi  = K.LL_GetBoneInstance(id);
			if(!bi.callback() || bi.callback_type() != bctPhysics  )
			{
				CPHElement *root_e = get_physics_parent( K, id );
				if( root_e && K.LL_GetBoneVisible( id ) )
					bi.set_callback( bctPhysics, 0, cast_PhysicsElement( root_e ) );
			}
		}
	};
	for_each_bone_id( *PKinematics(), set_bone_reference( *PKinematics() ) );
	
	//element_position_in_set_calbacks=u16(-1);
	
	//SetCallbacksRecursive(m_pKinematics->LL_GetBoneRoot(),element_position_in_set_calbacks);
}



void CPHShell::SetCallbacksRecursive(u16 id,u16 element)
{
	VERIFY( false );
	//if(elements.size()==element)	return;
	CBoneInstance& B	= m_pKinematics->LL_GetBoneInstance(u16(id));
	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));
	SJointIKData& joint_data=bone_data.IK_data;
	Flags64 mask;
	mask.assign(m_pKinematics->LL_GetBonesVisible());
	if(mask.is(1ui64<<(u64)id))
	{
		if((no_physics_shape(bone_data.shape)||joint_data.type==jtRigid)	&& element!=u16(-1)){
			B.set_callback(bctPhysics,0,cast_PhysicsElement(elements[element]));
		}else{
			element_position_in_set_calbacks++;
			element=element_position_in_set_calbacks;
			R_ASSERT2(element<elements.size(),"Out of elements!!");
			//if(elements.size()==element)	return;
			B.set_callback(bctPhysics,BonesCallback,cast_PhysicsElement(elements[element]));
			//B.Callback_overwrite=TRUE;
		}
	}

	for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
		SetCallbacksRecursive((*it)->GetSelfID(),element);
}

void CPHShell::ZeroCallbacks()
{
	if (m_pKinematics) ZeroCallbacksRecursive(m_pKinematics->LL_GetBoneRoot());
}
void CPHShell::ZeroCallbacksRecursive(u16 id)
{
	CBoneInstance& B	= m_pKinematics->LL_GetBoneInstance(u16(id));
	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));
	if(B.callback_type() == bctPhysics)
	{
		B.reset_callback	();
	}
	for (vecBonesIt it=bone_data.children.begin(); bone_data.children.end() != it; ++it)
		ZeroCallbacksRecursive		((*it)->GetSelfID());

}
void CPHShell::set_DynamicLimits(float l_limit,float w_limit)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->set_DynamicLimits(l_limit,w_limit);
}

void CPHShell::set_DynamicScales(float l_scale/* =default_l_scale */,float w_scale/* =default_w_scale */)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->set_DynamicScales(l_scale,w_scale);
}

void CPHShell::set_DisableParams(const SAllDDOParams& params)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->set_DisableParams(params);
}

void CPHShell::UpdateRoot()
{

	ELEMENT_I i=elements.begin();
	if( !(*i)->isFullActive()) return;

	(*i)->InterpolateGlobalTransform(&mXFORM);

}

Fmatrix& CPHShell::get_animation_root_matrix( Fmatrix& m )
{

	return m;
}

void CPHShell::InterpolateGlobalTransform(Fmatrix* m)
{
	
	//if(!CPHObject::is_active()&&!CPHObject::NetInterpolation()) return;

	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->InterpolateGlobalTransform(&(*i)->mXFORM);

	m->set( root_element().mXFORM );

	m->mulB_43	(m_object_in_root);

	mXFORM.set(*m);

	VERIFY2(_valid(*m),"not valide transform");

	CPhysicsShellHolder* ref_object=(*elements.begin())->PhysicsRefObject();
	if(ref_object&&m_active_count<0)
	{
		ref_object->processing_deactivate();
		ref_object->spatial_move();
		m_active_count=0;
	}

}

void CPHShell::GetGlobalTransformDynamic(Fmatrix* m) 
{
	ELEMENT_CI i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->GetGlobalTransformDynamic(&(*i)->mXFORM);
	m->set((*elements.begin())->mXFORM);

	m->mulB_43	(m_object_in_root);
	mXFORM.set(*m);

	VERIFY2(_valid(*m),"not valide transform");

}
void CPHShell::InterpolateGlobalPosition(Fvector* v)
{
	(*elements.begin())->InterpolateGlobalPosition(v);

	v->add(m_object_in_root.c);

	VERIFY2(_valid(*v),"not valide result position");
}

void CPHShell::GetGlobalPositionDynamic(Fvector* v)
{
	(*elements.begin())->GetGlobalPositionDynamic(v);
	VERIFY2(_valid(*v),"not valide result position");
}


void CPHShell::ObjectToRootForm(const Fmatrix& form)
{
	Fmatrix M;
	Fmatrix ILF;
	(*elements.begin())->InverceLocalForm(ILF);
	M.mul(m_object_in_root,ILF);
	M.invert();
	mXFORM.mul(form,M);
	VERIFY2(_valid(form),"not valide transform");
	
}



CPhysicsElement* CPHShell::NearestToPoint(const Fvector& point, NearestToPointCallback *cb /*=0*/)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	float min_distance	=dInfinity;
	CPHElement* nearest_element=NULL;
	for( ; i!=e; ++i )
	{
		Fvector tmp;
		float	distance;
		if( cb && ! (*cb)( *i ) )
			continue;
		(*i)->GetGlobalPositionDynamic(&tmp);
		tmp.sub(point);
		distance=tmp.magnitude();
		if(distance<min_distance)
		{
			min_distance=distance;
			nearest_element=*i;
		}
	}
	return nearest_element;
}
void CPHShell::CreateSpace()
{
	if(!m_space) 
	{
		m_space=dSimpleSpaceCreate(0);
		dSpaceSetCleanup (m_space, 0);
	}
}
void CPHShell::PassEndElements(u16 from,u16 to,CPHShell *dest)
{

	ELEMENT_I i_from=elements.begin()+from,e=elements.begin()+to;
	if(from!=to)
	{
		if(!dest->elements.empty())	(*i_from)->set_ParentElement(dest->elements.back());
		else						(*i_from)->set_ParentElement(NULL);
	}
	for(ELEMENT_I i=i_from;i!=e;++i)
	{
		dGeomID spaced_geom=(*i)->dSpacedGeometry();
		if(spaced_geom)//for active elems
		{
			dSpaceRemove (m_space,spaced_geom );
			dSpaceAdd(dest->m_space,spaced_geom);
		}
		VERIFY(_valid(dest->mXFORM));
		(*i)->SetShell(dest);
	}
	dest->elements.insert(dest->elements.end(),i_from,e);
	elements.erase(i_from,e);		

}

void CPHShell::PassEndJoints(u16 from,u16 to,CPHShell *dest)
{
	JOINT_I i_from=joints.begin()+from,e=joints.begin()+to;
	JOINT_I i=i_from;
	for(;i!=e;i++)
	{
		(*i)->SetShell(dest);
	}
	dest->joints.insert(dest->joints.end(),i_from,e);
	joints.erase(i_from,e);
}

void CPHShell::DeleteElement(u16 element)
{
	elements[element]->Deactivate();
	xr_delete(elements[element]);
	elements.erase(elements.begin()+element);
}
void CPHShell::DeleteJoint(u16 joint)
{
	joints[joint]->Deactivate();
	xr_delete(joints[joint]);
	joints.erase(joints.begin()+joint);
}

void CPHShell::setEndElementSplitter()
{

	if(!elements.back()->FracturesHolder())//adding fracture for element supposed before adding splitter. Need only one splitter for an element
		AddSplitter(CPHShellSplitter::splElement,u16(elements.size()-1),u16(joints.size()-1));
}

void CPHShell::setElementSplitter(u16 element_number,u16 splitter_position)
{
	AddSplitter(CPHShellSplitter::splElement,element_number,element_number-1,splitter_position);
}
void CPHShell::AddSplitter(CPHShellSplitter::EType type,u16 element,u16 joint)
{
	if(!m_spliter_holder) m_spliter_holder=xr_new<CPHShellSplitterHolder>(this);
	m_spliter_holder->AddSplitter(type,element,joint);
}

void CPHShell::AddSplitter(CPHShellSplitter::EType type,u16 element,u16 joint,u16 position)
{
	if(!m_spliter_holder) m_spliter_holder=xr_new<CPHShellSplitterHolder>(this);
	m_spliter_holder->AddSplitter(type,element,joint,position);
}
void CPHShell::setEndJointSplitter()
{
	if(!joints.back()->JointDestroyInfo())//setting joint breacable supposed before adding splitter. Need only one splitter for a joint
		AddSplitter(CPHShellSplitter::splJoint,u16(elements.size()-1),u16(joints.size()-1));
}

bool CPHShell::isBreakable()
{
	return (m_spliter_holder&&!m_spliter_holder->IsUnbreakable());
}

bool CPHShell::isFractured()
{
	return(m_spliter_holder&&m_spliter_holder->Breaked());
}

void CPHShell::SplitProcess(PHSHELL_PAIR_VECTOR &out_shels)
{
	if(! m_spliter_holder) return;
	m_spliter_holder->SplitProcess(out_shels);
	if(!m_spliter_holder->m_splitters.size()) xr_delete(m_spliter_holder);
}

u16 CPHShell::BoneIdToRootGeom(u16 id)
{
	if(! m_spliter_holder)return u16(-1);
	return m_spliter_holder->FindRootGeom(id);
}

void CPHShell::SetJointRootGeom(CPhysicsElement* root_e,CPhysicsJoint* J)
{
	R_ASSERT(root_e);
	R_ASSERT( J );
	CPHElement* e=cast_PHElement(root_e);
	CPHJoint*	j=static_cast<CPHJoint*>(J);

	CPHFracturesHolder* f_holder=e->FracturesHolder();
	if(!f_holder) return;
	j->RootGeom()=e->Geom(f_holder->LastFracture().m_start_geom_num);
}

void CPHShell::set_ApplyByGravity(bool flag)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->set_ApplyByGravity(flag);
}

bool CPHShell::get_ApplyByGravity()
{
	if (elements.empty())
		return	(false);

	VERIFY		(elements.front());
	return		(elements.front()->get_ApplyByGravity());
}

void CPHShell::applyGravityAccel(const Fvector& accel)
{
	if(!isActive())return;
	ELEMENT_I i,e;
	Fvector a;
	a.set(accel);
	a.mul((float)elements.size());
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->applyGravityAccel(a);
	EnableObject(0);
}

void CPHShell::PlaceBindToElForms()
{
	Flags64 mask;
	mask.assign(m_pKinematics->LL_GetBonesVisible());
	PlaceBindToElFormsRecursive(Fidentity,m_pKinematics->LL_GetBoneRoot(),0,mask);
}
void	CPHShell::		setTorque				(const Fvector& torque)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->setTorque(torque);
}
void	CPHShell::		setForce				(const Fvector& force)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
		(*i)->setForce(force);
}
void CPHShell::PlaceBindToElFormsRecursive(Fmatrix parent,u16 id,u16 element,Flags64 &mask)
{
	
	
	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));
	SJointIKData& joint_data=bone_data.IK_data;

	if(mask.is(1ui64<<(u64)id))
	{

		if(no_physics_shape(bone_data.shape)||joint_data.type==jtRigid&& element!=u16(-1))
		{

			
		}
		else
		{

			element++;
			R_ASSERT2(element<elements.size(),"Out of elements!!");
			//if(elements.size()==element)	return;
			CPHElement* E=(elements[element]);
			E->mXFORM.mul(parent,bone_data.bind_transform);
			
		}
	}
	for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
		PlaceBindToElFormsRecursive(mXFORM,(*it)->GetSelfID(),element,mask);

}

void CPHShell::BonesBindCalculate(u16 id_from)
{
	BonesBindCalculateRecursive(Fidentity,0);
}
void CPHShell::BonesBindCalculateRecursive(Fmatrix parent,u16 id)
{

	CBoneInstance& bone_instance=m_pKinematics->LL_GetBoneInstance(id);
	CBoneData& bone_data= m_pKinematics->LL_GetData(u16(id));

	bone_instance.mTransform.mul(parent,bone_data.bind_transform);
	
	for (vecBonesIt it=bone_data.children.begin(); it!=bone_data.children.end(); ++it)
		BonesBindCalculateRecursive(bone_instance.mTransform,(*it)->GetSelfID());
}

void CPHShell::AddTracedGeom				(u16 element/*=0*/,u16 geom/*=0*/)
{
	CODEGeom* g=elements[element]->Geom(geom);
	g->set_ph_object(this);
	m_traced_geoms.add(g);
	EnableGeomTrace	();
}
void CPHShell::SetAllGeomTraced()
{
	ELEMENT_I i, e, b;
	b = elements.begin(); e=elements.end();
	for( i=b ;i!=e; ++i )
	{
		u16 gn=(*i)->numberOfGeoms();
		for(u16 j=0;j<gn;++j)
			AddTracedGeom( u16(i-b), j );
	}
	
}

void		CPHShell::		ClearTracedGeoms()
{
	m_traced_geoms.clear();
	DisableGeomTrace();
}

void		CPHShell::		DisableGeomTrace()
{
	CPHObject::UnsetRayMotions();
}
void		CPHShell::		EnableGeomTrace	()
{
	if( !m_traced_geoms.empty() )
			CPHObject::SetRayMotions();
}


void CPHShell::SetPrefereExactIntegration()
{
	CPHObject::SetPrefereExactIntegration();
}

void CPHShell::add_Element					(CPhysicsElement* E)		  {
	CPHElement* ph_element=cast_PHElement(E);
	ph_element->SetShell(this);
	elements.push_back(ph_element);

}

void CPHShell::add_Joint					(CPhysicsJoint* J)					{
	if(!J)return;
	joints.push_back(static_cast<CPHJoint*>(J));
	joints.back()->SetShell(this);
}

CODEGeom* CPHShell::get_GeomByID(u16 bone_id)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)
	{
		CODEGeom* ret=(*i)->GeomByBoneID(bone_id);
		if(ret) return ret;
	}
	return NULL;
}
void	CPHShell::PureStep(float step)
{
	//CPHObject::Island().Step(step);
	//PhDataUpdate(step);
	CPHObject::step( step );
}
void CPHShell::CollideAll()
{
	CPHObject::Collide();
	CPHObject::reinit_single();
}

void	CPHShell::RegisterToCLGroup	(CGID g)	
{
	CPHCollideValidator::RegisterObjToGroup(g,*static_cast<CPHObject*>(this));
}

bool CPHShell::IsGroupObject()
{
	return CPHCollideValidator::IsGroupObject(*this);
};

void CPHShell::SetIgnoreStatic()
{
	CPHCollideValidator::SetStaticNotCollide(*this);
}

void CPHShell::SetIgnoreDynamic()
{
	CPHCollideValidator::SetDynamicNotCollide(*this);
}

void CPHShell::SetRagDoll()
{
	CPHCollideValidator::SetRagDollClass(*this);
}

void CPHShell::SetIgnoreRagDoll()
{
	CPHCollideValidator::SetRagDollClassNotCollide(*this);
}


	//Делает данный физический объек анимированным 
void CPHShell::CreateShellAnimator( CInifile* ini, LPCSTR section )
{	
	//Для фильтра коллизий относим данный объект к классу анимированных
	CPHCollideValidator::SetAnimatedClass(*this);
	m_pPhysicsShellAnimatorC=xr_new<CPhysicsShellAnimator>( this,  ini, section );
	VERIFY( PhysicsRefObject( ) );
	PhysicsRefObject()->processing_activate();
	//m_pPhysicsShellAnimatorC->ResetCallbacks();
}

//Настраивает фильтр коллизий на игнорирование столкновенний данного
//физического объекта с анимированным физическим объектом
void CPHShell::SetIgnoreAnimated()
{
	//Для фильтра коллизий указываем, что данный
	//физический объект игнорирует анимированные физические тела
	
	CPHCollideValidator::SetAnimatedClassNotCollide(*this);
}

//Выдает информацию о том является ли данный объект анимированным


void	CPHShell::				SetSmall()
{
	CPHCollideValidator::SetClassSmall(*this);
}

void	CPHShell::				SetIgnoreSmall()
{
	CPHCollideValidator::SetClassSmallNotCollide(*this);
}
void CPHShell::CutVelocity(float l_limit,float a_limit)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)(*i)->CutVelocity(l_limit,a_limit);
}

void CPHShell::ClearRecentlyDeactivated()
{
	ClearCashedTries();
}

void CPHShell::ClearCashedTries()
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)(*i)->clear_cashed_tries();
}

void CPHShell::get_Extensions(const Fvector& axis,float center_prg,float& lo_ext, float& hi_ext) const
{
	t_get_extensions( elements, axis, center_prg, lo_ext, hi_ext );
	/*
	lo_ext=dInfinity;hi_ext=-dInfinity;
	ELEMENT_CI i=elements.begin(),e=elements.end();
	for(;i!=e;++i)
	{
		float temp_lo_ext,temp_hi_ext;
		(*i)->get_Extensions(axis,center_prg,temp_lo_ext,temp_hi_ext);
		if(lo_ext>temp_lo_ext)lo_ext=temp_lo_ext;
		if(hi_ext<temp_hi_ext)hi_ext=temp_hi_ext;
	}
*/
}

const	CGID&	CPHShell::GetCLGroup										()const
{
	return CPHCollideValidator::GetGroup(*this);
}

void*		CPHShell::				get_CallbackData						()
{
	VERIFY(isActive());
	return	(*elements.begin())->get_CallbackData();
}

void	CPHShell::		SetBonesCallbacksOverwrite(bool v)
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ;i!=e;++i)	
		(*i)->SetBoneCallbackOverwrite(v);
}


void		CPHShell::	ToAnimBonesPositions	()
{
	VERIFY( PKinematics() );
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	for( ; i!=e; ++i )	
		(*i)->ToBonePos( &PKinematics()->LL_GetBoneInstance( (*i)->m_SelfID ) );
}

bool	CPHShell::		AnimToVelocityState		( float dt, float l_limit, float a_limit )
{
	ELEMENT_I i,e;
	i=elements.begin(); e=elements.end();
	bool ret = true;
	for( ;i!=e; ++i )
		ret  =(*i)->AnimToVel( dt, l_limit, a_limit ) && ret;
	return ret;	
}

void	CPHShell::SetAnimated( bool v )
{
	ELEMENT_I i, e;
	i = elements.begin(); e=elements.end();
	for( ; i!=e; ++i )
		(*i)->SetAnimated( v );
	
}




#ifdef DEBUG
void		CPHShell::		dbg_draw_velocity				( float scale, u32 color )
{
		ELEMENT_I i, e;
	i = elements.begin(); e=elements.end();
	for( ; i!=e; ++i )
		(*i)->dbg_draw_velocity( scale, color );

}
void		CPHShell::		dbg_draw_force				( float scale, u32 color )
{
	ELEMENT_I i, e;
	i = elements.begin(); e=elements.end();
	for( ; i!=e; ++i )
		(*i)->dbg_draw_force( scale, color );
}

void CPHShell::dbg_draw_geometry	( float scale, u32 color, Flags32 flags /*= Flags32().assign( 0 )*/ ) const
{
	ELEMENT_CI i, e;
	i = elements.begin(); e=elements.end();
	for( ; i!=e; ++i )
		(*i)->dbg_draw_geometry( scale, color, flags );
}

#endif
