#include "stdafx.h"

#include "PHWorld.h"
#include "tri-colliderknoopc/dTriList.h"
#include "PhysicsCommon.h"
#include "Level.h"
#include "ExtendedGeom.h"
#include "draymotions.h"
#include "PHCollideValidator.h"
#include "../xrEngine/gamemtllib.h"
#ifdef    DEBUG
#	include "PHDebug.h"
#endif
#include "PHCommander.h"
#include "PHSimpleCalls.h"
#include "PHSynchronize.h"
#include "phnetstate.h"
#include "geometrybits.h"

#ifdef	DEBUG
void DBG_ObjAfterPhDataUpdate	( CPHObject *obj );
void DBG_ObjBeforePhDataUpdate	( CPHObject *obj );
void DBG_ObjAfterStep			( CPHObject *obj );
void DBG_ObjBeforeStep			( CPHObject *obj );
void DBG_ObjeAfterPhTune		( CPHObject *obj );
void DBG_ObjBeforePhTune		( CPHObject *obj );
void DBG_ObjAfterCollision		( CPHObject *obj );
void DBG_ObjBeforeCollision		( CPHObject *obj );
#endif
//////////////////////////////////////////////////////////////
//////////////CPHMesh///////////////////////////////////////////
///////////////////////////////////////////////////////////
BOOL g_bDebugDumpPhysicsStep				=					0;

void CPHMesh ::Create(dSpaceID space, dWorldID world){
	Geom = dCreateTriList(space, 0, 0);
	CPHGeometryBits::init_geom( *this );
}
/////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////

void CPHMesh ::Destroy(){
	
	dGeomDestroy(Geom);
	dTriListClass=-1;

}



////////////////////////////////////////////////////////////////////////////
///////////CPHWorld/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//#define PH_PLAIN
#ifdef PH_PLAIN
dGeomID plane;
#endif

#ifdef DEBUG 

void CPHWorld::OnRender()
{

	PH_DBG_Render();
}
#endif
CPHWorld::CPHWorld()
{
	disable_count=0;
	m_frame_time=0.f;
	m_previous_frame_time=0.f;
	b_frame_mark=false;
	m_steps_num=0;
	m_steps_short_num = 0;
	m_frame_sum=0.f;
	m_delay=0; 
	m_previous_delay=0;
	m_reduce_delay=0;
	m_update_delay_count=0;
	b_world_freezed=false;
	b_processing=false;
	m_gravity	=default_world_gravity;
	b_exist=false;
}
void CPHWorld::SetStep(dReal s)
{
	fixed_step											=	s;
	world_cfm											=	CFM(SPRING_S(base_cfm,base_erp,base_fixed_step),DAMPING(base_cfm,base_erp));
	world_erp											=	ERP(SPRING_S(base_cfm,base_erp,base_fixed_step),DAMPING(base_cfm,base_erp));
	world_spring										=	1.0f*SPRING	(world_cfm,world_erp);
	world_damping										=	1.0f*DAMPING(world_cfm,world_erp);
	if(ph_world&&ph_world->Exist())
	{
		float	frame_time					=	Device.fTimeDelta;
		u32		it_number					=	iFloor	(frame_time /fixed_step);
		frame_time							-=	it_number*fixed_step;
		ph_world->m_previous_frame_time		=	frame_time;
		ph_world->m_frame_time				=	frame_time;
	}
}
void CPHWorld::Create()
{
	dWorldID phWorld=0;
	
	if (psDeviceFlags.test(mtPhysics))	Device.seqFrameMT.Add	(this,REG_PRIORITY_HIGH);
	else								Device.seqFrame.Add		(this,REG_PRIORITY_LOW);
	
	m_commander							=xr_new<CPHCommander>();
	//dVector3 extensions={2048,256,2048};
	/*
	Fbox	level_box		=	Level().ObjectSpace.GetBoundingVolume();
	Fvector level_size,level_center;
	level_box				.	getsize		(level_size);
	level_box				.	getcenter	(level_center);
	dVector3 extensions		=	{ level_size.x ,256.f,level_size.z};
	dVector3 center			=	{level_center.x,0.f,level_center.z};
	*/

#ifdef ODE_SLOW_SOLVER
#else

	dWorldSetAutoEnableDepthSF1(phWorld, 100000000);
	///dWorldSetContactSurfaceLayer(phWorld,0.f);
	//phWorld->contactp.min_depth =0.f;

#endif
	ContactGroup			= dJointGroupCreate(0);		
	dWorldSetGravity		(phWorld, 0,-Gravity(), 0);//-2.f*9.81f
	Mesh.Create				(0,phWorld);
#ifdef PH_PLAIN
	plane=dCreatePlane(Space,0,1,0,0.3f);
#endif

	//const  dReal k_p=2400000.f;//550000.f;///1000000.f;
	//const dReal k_d=200000.f;
	dWorldSetERP(phWorld, ERP(world_spring,world_damping) );
	dWorldSetCFM(phWorld, CFM(world_spring,world_damping));
	//dWorldSetERP(phWorld,  0.2f);
	//dWorldSetCFM(phWorld,  0.000001f);
	disable_count=0;
	m_motion_ray=dCreateRayMotions(0);
	phBoundaries.set(Level().ObjectSpace.GetBoundingVolume());
	phBoundaries.y1-=30.f;
	CPHCollideValidator::Init();
	b_exist=true;
}

/////////////////////////////////////////////////////////////////////////////

void CPHWorld::Destroy()
{
	r_spatial.clear();
	xr_delete(m_commander);
	Mesh.Destroy();
#ifdef PH_PLAIN
	dGeomDestroy(plane);
#endif
#ifdef DEBUG
	PH_DBG_Clear();
#endif
	dGeomDestroy(m_motion_ray);
	dJointGroupEmpty(ContactGroup);
	dJointGroupDestroy(ContactGroup);
	ContactFeedBacks.clear();
	ContactEffectors.clear();
	dCloseODE();
	dCylinderClassUser=-1;
	dRayMotionsClassUser=-1;
	Device.seqFrameMT.Remove	(this);
	Device.seqFrame.Remove		(this);
	b_exist=false;
}
void CPHWorld::SetGravity(float g)
{
	m_gravity				=g;
	dWorldID phWorld		=0;
	dWorldSetGravity		(phWorld, 0,-m_gravity, 0);//-2.f*9.81f

}

void CPHWorld::OnFrame()
{
	// Msg									("------------- physics: %d / %d",u32(Device.dwFrame),u32(m_steps_num));
	//просчитать полет пуль
	/*
	Device.Statistic->TEST0.Begin		();
	Level().BulletManager().Update		();
	Device.Statistic->TEST0.End			();
	*/
#ifdef DEBUG 
	//DBG_DrawFrameStart();
	//DBG_DrawStatBeforeFrameStep();
#endif
	Device.Statistic->Physics.Begin		();
	FrameStep							(Device.fTimeDelta);
	Device.Statistic->Physics.End		();
#ifdef DEBUG
	//DBG_DrawStatAfterFrameStep();

#endif
}

//////////////////////////////////////////////////////////////////////////////
//static dReal frame_time=0.f;
static u32 start_time=0;
void CPHWorld::Step()
{
#ifdef DEBUG
	dbg_reused_queries_per_step	=0			;
	dbg_new_queries_per_step	=0			;
#endif
	
	VERIFY(b_processing||IsFreezed());

	PH_OBJECT_I			i_object;
	PH_UPDATE_OBJECT_I	i_update_object;

	if(disable_count==0)
	{
		disable_count=worldDisablingParams.objects_params.L2frames;
		for(i_object=m_recently_disabled_objects.begin();m_recently_disabled_objects.end() != i_object;)
		{
			CPHObject* obj=(*i_object);
			obj->check_recently_deactivated();
			++i_object;
		}
	}
	if(!IsFreezed())
		--disable_count;

	++m_steps_num;
	++m_steps_short_num;
	Device.Statistic->ph_collision.Begin	();


	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
#ifdef	DEBUG
		DBG_ObjBeforeCollision( obj );
#endif
		obj->Collide();
#ifdef	DEBUG
		DBG_ObjAfterCollision( obj );
#endif
		++i_object;
	}



	Device.Statistic->ph_collision.End	();

#ifdef DEBUG
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		if( ph_dbg_draw_mask.test(phDbgDrawEnabledAABBS) )
		DBG_DrawPHObject(obj);
		++i_object;
	}
#endif

	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{	
		CPHObject* obj=(*i_object);
		++i_object;

#ifdef	DEBUG
		DBG_ObjBeforePhTune( obj );
#endif

		obj->PhTune(fixed_step);

#ifdef	DEBUG
		DBG_ObjeAfterPhTune( obj );
#endif

	}

	for(i_update_object=m_update_objects.begin();m_update_objects.end() != i_update_object;)
	{	CPHUpdateObject* obj=(*i_update_object);
		++i_update_object;
		obj->PhTune(fixed_step);
	}

	Device.Statistic->ph_core.Begin		();

#ifdef DEBUG
	dbg_bodies_num=0;
	dbg_joints_num=0;
	dbg_islands_num=0;
#endif
//////////////////////////////////////////////////////////////////////
	m_commander						->update();
//////////////////////////////////////////////////////////////////////
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{	
		CPHObject* obj=(*i_object);
		++i_object;
#ifdef DEBUG
		if(ph_dbg_draw_mask.test(phDbgDrawObjectStatistics))
		{
			if(obj->Island().IsActive())
			{
				dbg_islands_num++;
				dbg_joints_num+=obj->Island().nj;
				dbg_bodies_num+=obj->Island().nb;
			}
		}
#endif

#ifdef	DEBUG
		DBG_ObjBeforeStep( obj );
#endif
		obj->IslandStep(fixed_step);

#ifdef	DEBUG
		DBG_ObjAfterStep( obj );
#endif
	}

	Device.Statistic->ph_core.End		();



	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		++i_object;
		obj->IslandReinit();

#ifdef	DEBUG
		DBG_ObjBeforePhDataUpdate( obj );
#endif

		obj->PhDataUpdate(fixed_step);

#ifdef	DEBUG
		DBG_ObjAfterPhDataUpdate( obj );
#endif

		obj->spatial_move();
	}

	for(i_update_object=m_update_objects.begin();m_update_objects.end() != i_update_object;)
	{	
		CPHUpdateObject* obj=*i_update_object;
		++i_update_object;
		obj->PhDataUpdate(fixed_step);
	}



#ifdef DEBUG
	dbg_contacts_num=ContactGroup->num;
#endif
	dJointGroupEmpty(ContactGroup);//this is to be called after PhDataUpdate!!!-the order is critical!!!
	ContactFeedBacks.empty();
	ContactEffectors.empty();



	if(physics_step_time_callback) 
	{
		physics_step_time_callback(start_time,start_time+u32(fixed_step*1000));	
		start_time += u32(fixed_step*1000);
	};
}

void CPHWorld::StepTouch()
{
	PH_OBJECT_I			i_object;
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		obj->Collide();

		++i_object;
	}

	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		++i_object;
		obj->Island().Enable();
	}
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		++i_object;
		obj->IslandReinit();
		obj->spatial_move();
	}
	dJointGroupEmpty(ContactGroup);
	ContactFeedBacks.empty();
	ContactEffectors.empty();
}

u32 CPHWorld::CalcNumSteps (u32 dTime)
{
	if (dTime < m_frame_time*1000) return 0;
	u32 res = iCeil((float(dTime) - m_frame_time*1000) / (fixed_step*1000));
//	if (dTime < fixed_step*1000) return 0;
//	u32 res = iFloor((float(dTime) / 1000 / fixed_step)+0.5f);
	return res;
};

void CPHWorld::FrameStep(dReal step)
{
	if(IsFreezed())		return;
	
	VERIFY		(_valid(step))	;
	step	*=	phTimefactor	;
	// compute contact joints and forces

	//step+=astep;

	//const  dReal k_p=24000000.f;//550000.f;///1000000.f;
	//const dReal k_d=400000.f;
	u32 it_number;
	float frame_time=m_frame_time;
	frame_time+=step;
	//m_frame_sum+=step;
#ifdef DEBUG
	if(ph_dbg_draw_mask.test(phDbgDrawObjectStatistics))
	{
		static float dbg_iterations=0.f;
		dbg_iterations=dbg_iterations*0.9f+step/fixed_step*0.1f;
		b_processing=true;
		DBG_OutText("phys steps per frame %2.1f",dbg_iterations);
		b_processing=false;
	}
#endif
	if(!(frame_time<fixed_step))
	{
		it_number				=	iFloor	(frame_time/fixed_step);
		frame_time				-=	it_number*fixed_step;
		m_previous_frame_time	=	m_frame_time;
		m_frame_time			=	frame_time;
		b_frame_mark			=	!b_frame_mark;
	}
	else
	{
		m_frame_time=	frame_time;
		return		;
	}
	//for(UINT i=0;i<(m_reduce_delay+1);++i)
#ifdef DEBUG 
	DBG_DrawFrameStart();
	DBG_DrawStatBeforeFrameStep();
#endif
	b_processing=true;

	start_time = Device.dwTimeGlobal;// - u32(m_frame_time*1000);
	if(g_bDebugDumpPhysicsStep&&it_number>20)Msg("!!!TOO MANY PHYSICS STEPS PER FRAME = %d !!!",it_number);
	for(UINT i=0; i<it_number;++i)	Step();
	b_processing=false;
#ifdef DEBUG
	DBG_DrawStatAfterFrameStep();
#endif
}

void CPHWorld::AddObject(CPHObject* object){
	m_objects.push_back(object);
	//xr_list <CPHObject*> ::iterator i= m_objects.end();
	//return (--m_objects.end());
};
void CPHWorld::AddRecentlyDisabled(CPHObject* object)
{
	m_recently_disabled_objects.push_back(object);
}
void CPHWorld::RemoveFromRecentlyDisabled(PH_OBJECT_I i)
{
	m_recently_disabled_objects.erase(i);
}

void CPHWorld::AddUpdateObject(CPHUpdateObject* object)
{
//.	if(object->IsFreezed())m_freezed_update_objects.erase(i);
	m_update_objects.push_back(object);
}

void CPHWorld::RemoveUpdateObject(PH_UPDATE_OBJECT_I i)
{
	m_update_objects.erase(i);
}

void CPHWorld::RemoveObject(PH_OBJECT_I i){
	m_objects.erase((i));
};

void CPHWorld::AddFreezedObject(CPHObject* obj)
{
	m_freezed_objects.push_back(obj);
}

void CPHWorld::RemoveFreezedObject(PH_OBJECT_I i)
{
	m_freezed_objects.erase(i);
}
void CPHWorld::Freeze()
{
	R_ASSERT2(!b_world_freezed,"already freezed!!!");
	m_freezed_objects.move_items(m_objects);

	PH_OBJECT_I iter=m_freezed_objects.begin(),
		e=	m_freezed_objects.end()	;



	for(; e != iter;++iter)
		(*iter)->FreezeContent();
	
	m_freezed_update_objects.move_items(m_update_objects);

	b_world_freezed=true;
}
void CPHWorld::UnFreeze()
{
	R_ASSERT2(b_world_freezed,"is not freezed!!!");
	PH_OBJECT_I iter=m_freezed_objects.begin(),
		e=	m_freezed_objects.end()	;
	for(; e != iter;++iter)
		(*iter)->UnFreezeContent();

	m_objects.move_items(m_freezed_objects);

	m_update_objects.move_items(m_freezed_update_objects);
	b_world_freezed=false;
}
bool CPHWorld::IsFreezed()
{
	return b_world_freezed;
}

void CPHWorld::CutVelocity(float l_limit,float a_limit)
{
	PH_OBJECT_I			i_object;
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		obj->CutVelocity(l_limit,a_limit);
		++i_object;
	}
}
void CPHWorld::NetRelcase(CPhysicsShell *s)
{
	CPHReqComparerHasShell c(s);
	m_commander->remove_calls(&c);

	PH_UPDATE_OBJECT_I	i_update_object;
	for(i_update_object=m_update_objects.begin();m_update_objects.end() != i_update_object;)
	{	CPHUpdateObject* obj=(*i_update_object);
		++i_update_object;
		obj->NetRelcase( s );
		//obj->PhTune(fixed_step);
	}
	
}
void CPHWorld::AddCall(CPHCondition*c,CPHAction*a)
{
	m_commander->add_call(c,a);
}
u16 CPHWorld::ObjectsNumber()
{
	return m_objects.count();
}
u16 CPHWorld::UpdateObjectsNumber()
{
	return m_update_objects.count();
}
void CPHWorld::GetState(V_PH_WORLD_STATE& state)
{
	state.clear();
	PH_OBJECT_I			i_object;
	for(i_object=m_objects.begin();m_objects.end() != i_object;)
	{
		CPHObject* obj=(*i_object);
		const u16 els=obj->get_elements_number();
		for(u16 i=0;els>i;++i)
		{
			std::pair<CPHSynchronize*,SPHNetState> s;
			s.first=obj->get_element_sync(i);
			s.first->get_State(s.second);
			state.push_back(s);
		}
		++i_object;
	}	
}