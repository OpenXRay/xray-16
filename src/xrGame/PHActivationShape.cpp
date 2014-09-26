#include "stdafx.h"
#include "PHObject.h"
#include "Physics.h"
#include "MathUtils.h"
#include "phvalidevalues.h"
#include "PHActivationShape.h"
#include "Extendedgeom.h"
#include "SpaceUtils.h"
#include "MathUtils.h"
#include "../xrEngine/gamemtllib.h"
#include "Level.h"
#include "PHWorld.h"
#include "../3rd party/ode/ode/src/util.h"

#ifdef DEBUG
#	include "PHDebug.h"
#endif // DEBUG

#include "PHDynamicData.h"
#include "PHSynchronize.h"
#include "phnetstate.h"
static	float max_depth			=0.f;
static	float friction_factor	=0.f;
static	const float cfm				=1.e-10f;
static	const float erp				=1.f;
/*
static	const float static_cfm				=1.e-10f;
static	const float static_erp				=1.f;

static	const float dynamic_cfm				= 1.f;//static_cfm;//
static	const float dynamic_erp				= 1.f / 1000.f;//static_erp;//
*/

#ifdef DEBUG
#define	CHECK_POS(pos,msg,br)			if (!valid_pos(pos,phBoundaries)){Msg("pos:%f,%f,%f",pos.x,pos.y,pos.z);Msg(msg);VERIFY(!br);}

#else
#define	CHECK_POS(pos,msg,br)				
#endif
extern int dTriListClass;

static void	ActivateTestDepthCallback (bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
{
	
	if(!do_colide||
		material_1->Flags.test(SGameMtl::flPassable) ||
		material_2->Flags.test(SGameMtl::flPassable)
	)
	return;

	float& depth=c.geom.depth;
	float test_depth=depth;
	save_max(max_depth,test_depth);
	c.surface.mu*=friction_factor;
	c.surface.soft_cfm=cfm;
	c.surface.soft_erp=erp;
/*


	VERIFY(dTriListClass != dGeomGetClass(c.geom.g2));
	bool cl_statics = (dTriListClass == dGeomGetClass(c.geom.g1));
	VERIFY( bo1 || cl_statics );
	CPHObject* self = static_cast<CPHObject*> ( ( (CPHActivationShape*)(retrieveGeomUserData( bo1 ? c.geom.g1 : c.geom.g2 )->callback_data) ) );
	VERIFY( self );
	
	if( cl_statics )
	{
		c.surface.soft_cfm=static_cfm;
		c.surface.soft_erp=static_erp;
		dJointID contact_joint	= dJointCreateContactSpecial( 0, ContactGroup, &c );
		self->DActiveIsland()->ConnectJoint(contact_joint);
		dJointAttach(contact_joint, dGeomGetBody(c.geom.g1), dGeomGetBody(c.geom.g2));
		do_colide = false;
		return;
	}

	c.surface.soft_cfm=dynamic_cfm;
	c.surface.soft_erp=dynamic_erp;
	
	dxGeomUserData* data_oposite =  retrieveGeomUserData( bo1 ? c.geom.g2 : c.geom.g1 );
	
	if( !data_oposite || !data_oposite->ph_object )
		return;
	
	CPHObject* obj1 = 0, *obj2 = 0;
	if(bo1)
	{
		obj1 = self;
		obj2 = data_oposite->ph_object;
	} else
	{
		obj2 = self;
		obj1 = data_oposite->ph_object;
	}
	
	do_colide = false;

	VERIFY( obj1 && obj2 );

	int max_contacts;
	if( !obj1->DActiveIsland()->CanMerge(obj2->DActiveIsland(),max_contacts ) ) 
		return;
	if( max_contacts < 1 )
		return;
	
	dJointID contact_joint	= dJointCreateContactSpecial( 0, ContactGroup, &c );
	obj1->DActiveIsland()->ConnectJoint(contact_joint);
	dJointAttach			(contact_joint, dGeomGetBody(c.geom.g1), dGeomGetBody(c.geom.g2));

	obj1->DActiveIsland()->Merge( obj2->DActiveIsland() );
	obj2->EnableObject( obj1 );
	*/

}

void	StaticEnvironment (bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
{
	dJointID contact_joint	= dJointCreateContact(0, ContactGroup, &c);

	if(bo1)
	{
		((CPHActivationShape*)(retrieveGeomUserData(c.geom.g1)->callback_data))->DActiveIsland()->ConnectJoint(contact_joint);
		dJointAttach			(contact_joint, dGeomGetBody(c.geom.g1), 0);
	}
	else
	{
		((CPHActivationShape*)(retrieveGeomUserData(c.geom.g2)->callback_data))->DActiveIsland()->ConnectJoint(contact_joint);
		dJointAttach			(contact_joint, 0, dGeomGetBody(c.geom.g2));
	}
	do_colide=false;
}
void  GetMaxDepthCallback (bool& do_colide,bool bo1,dContact& c,SGameMtl* material_1,SGameMtl* material_2)
{

	if(!do_colide ||
		material_1->Flags.test(SGameMtl::flPassable) ||
		material_2->Flags.test(SGameMtl::flPassable)
		)
		return;
	
	float& depth=c.geom.depth;
	float test_depth=depth;
	//save_max(max_depth,test_depth);
	max_depth+=test_depth;
}

void RestoreVelocityState(V_PH_WORLD_STATE& state)
{
	V_PH_WORLD_STATE::iterator i=state.begin(),e=state.end();	
	for(;e!=i;++i)
	{
		CPHSynchronize& sync=*i->first;
		SPHNetState&	old_s	=i->second;
		SPHNetState new_s;sync.get_State(new_s);
		new_s.angular_vel.set(old_s.angular_vel);
		new_s.linear_vel.set(old_s.linear_vel);
		new_s.enabled=old_s.enabled;
		sync.set_State(new_s);
	}
}

CPHActivationShape::CPHActivationShape()
{
	m_geom=NULL;
	m_body=NULL;
	m_flags.zero();
	m_flags.set(flFixedRotation,TRUE);
}
CPHActivationShape::~CPHActivationShape()
{
	VERIFY(!m_body&&!m_geom);
}
void	CPHActivationShape::Create(const Fvector start_pos,const Fvector start_size,CPhysicsShellHolder* ref_obj,EType _type/*=etBox*/,u16	flags)
{
	VERIFY(ref_obj);
	R_ASSERT(_valid( start_pos ) );
	R_ASSERT( _valid( start_size ) );

	m_body			=	dBodyCreate	(0)												;
	dMass m;
	dMassSetSphere(&m,1.f,100000.f);
	dMassAdjust(&m,1.f);
	dBodySetMass(m_body,&m);
	switch(_type)
	{
	case etBox:
	m_geom			=	dCreateBox	(0,start_size.x,start_size.y,start_size.z)		;
	break;

	case etSphere:
	m_geom			=	dCreateSphere	(0,start_size.x);
	break;
	};

	dGeomCreateUserData				(m_geom)										;
	dGeomUserDataSetObjectContactCallback(m_geom,ActivateTestDepthCallback)			;
	dGeomUserDataSetPhysicsRefObject(m_geom,ref_obj)								;
	dGeomSetBody					(m_geom,m_body)									;
	dBodySetPosition				(m_body,start_pos.x,start_pos.y,start_pos.z)	;
	Island()		.AddBody		(m_body)										;
	dBodyEnable						(m_body)										;
	m_safe_state					.create(m_body)									;
	spatial_register				()												;
	m_flags.set(flags,TRUE);
}
void CPHActivationShape::	Destroy	()
{
	VERIFY(m_geom&&m_body)				;
	spatial_unregister		()			;
	CPHObject::deactivate	()			;
	dGeomDestroyUserData	(m_geom)	;
	dGeomDestroy			(m_geom)	;
	m_geom					=NULL		;
	dBodyDestroy			(m_body)	;
	m_body					=NULL		;
}
bool	CPHActivationShape::	Activate							(const Fvector need_size,u16 steps,float max_displacement,float max_rotation,bool	un_freeze_later/*	=false*/)										
{

#ifdef	DEBUG 
	if(ph_dbg_draw_mask.test(phDbgDrawDeathActivationBox))
	{
		DBG_OpenCashedDraw();
		Fmatrix M;
		PHDynamicData::DMXPStoFMX(dBodyGetRotation(m_body),dBodyGetPosition(m_body),M);
		Fvector v;dGeomBoxGetLengths(m_geom,cast_fp(v));v.mul(0.5f);
		DBG_DrawOBB(M,v,D3DCOLOR_XRGB(0,255,0));
	}
#endif
	VERIFY(m_geom&&m_body);
	CPHObject::activate();
	ph_world->Freeze();
	UnFreeze();
	max_depth=0.f;

	dGeomUserDataSetObjectContactCallback(m_geom,GetMaxDepthCallback)			;
	//ph_world->Step();
	ph_world->StepTouch();	
	u16		num_it =15;
	float	fnum_it=float(num_it);
	float	fnum_steps=float(steps);
	float	fnum_steps_r=1.f/fnum_steps;
	float	resolve_depth=0.01f;
	float	max_vel=max_depth/fnum_it*fnum_steps_r/fixed_step;
	float	limit_l_vel=_max(_max(need_size.x,need_size.y),need_size.z)/fnum_it*fnum_steps_r/fixed_step;

	if(limit_l_vel>default_l_limit)
		limit_l_vel=default_l_limit;

	if(max_vel>limit_l_vel)
		max_vel=limit_l_vel;

	float	max_a_vel=max_rotation/fnum_it*fnum_steps_r/fixed_step;

	if(max_a_vel>default_w_limit)
					max_a_vel=default_w_limit;

	//ph_world->CutVelocity(0.f,0.f);
	dGeomUserDataSetCallbackData(m_geom,this);
	dGeomUserDataSetObjectContactCallback( m_geom, ActivateTestDepthCallback );
	if( m_flags.test( flStaticEnvironment ) )
		dGeomUserDataAddObjectContactCallback(m_geom,StaticEnvironment);
	max_depth=0.f;
	
	Fvector from_size;
	Fvector step_size,size;
	dGeomBoxGetLengths(m_geom,cast_fp(from_size));
	step_size.sub(need_size,from_size);
	step_size.mul(fnum_steps_r);
	size.set(from_size);
	bool ret=false;
	V_PH_WORLD_STATE temp_state;
	ph_world->GetState(temp_state);
	for(int m=0;steps>m;++m)
	{
		//float param =fnum_steps_r*(1+m);
		//InterpolateBox(id,param);
		size.add(step_size);
		dGeomBoxSetLengths(m_geom,size.x,size.y,size.z);
		u16		attempts=10;
		do{
		
			ret=false;
			for(int i=0;num_it>i;++i)
			{
				max_depth=0.f;
				ph_world->Step();
				CHECK_POS(Position(),"pos after ph_world->Step()",false);
				ph_world->CutVelocity(max_vel,max_a_vel);
				CHECK_POS(Position(),"pos after CutVelocity",true);
				//if(m==0&&i==0)ph_world->GetState(temp_state);
				if(max_depth	<	resolve_depth) 
				{
						ret=true;
						break;
				}
			}
			attempts--;
		}while(!ret&&attempts>0);
#ifdef	DEBUG
		Msg("correction attempts %d",10-attempts);
#endif
	
	}
	RestoreVelocityState(temp_state);
	CHECK_POS(Position(),"pos after RestoreVelocityState(temp_state);",true);
	if(!un_freeze_later)ph_world->UnFreeze();
#ifdef	DEBUG 
	if(ph_dbg_draw_mask.test(phDbgDrawDeathActivationBox))
	{
		DBG_OpenCashedDraw();
		Fmatrix M;
		PHDynamicData::DMXPStoFMX(dBodyGetRotation(m_body),dBodyGetPosition(m_body),M);
		Fvector v;v.set(need_size);v.mul(0.5f);
		DBG_DrawOBB(M,v,D3DCOLOR_XRGB(0,255,255));
		DBG_ClosedCashedDraw(30000);
	}
#endif
	return ret;
}
const Fvector&	CPHActivationShape::	Position							()																
{
	return cast_fv(dBodyGetPosition(m_body));
}
void	CPHActivationShape::	Size								(Fvector &size)																
{
	dGeomBoxGetLengths(m_geom,cast_fp(size));
}

void	CPHActivationShape::	PhDataUpdate						(dReal step)												
{
	m_safe_state.new_state(m_body);
}
void	CPHActivationShape::	PhTune								(dReal step)												
{

}
dGeomID	CPHActivationShape::	dSpacedGeom							()															
{
	return m_geom;
}
void	CPHActivationShape::	get_spatial_params					()															
{
	spatialParsFromDGeom(m_geom,spatial.sphere.P,AABB,spatial.sphere.R);
}

void	CPHActivationShape::	InitContact							(dContact* c,bool& do_collide,u16 ,u16 )
{

}

void CPHActivationShape::CutVelocity(float l_limit,float /*a_limit*/)
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

void	CPHActivationShape::		set_rotation						(const	Fmatrix	&sof)
{
	dMatrix3 rot;PHDynamicData::FMXtoDMX(sof,rot);
	dBodySetRotation(ODEBody(),rot);
	m_safe_state.set_rotation( rot );
}

#ifdef		DEBUG
CPhysicsShellHolder* CPHActivationShape::ref_object	()
{
	VERIFY(m_geom);
	dxGeomUserData* ud = retrieveGeomUserData( m_geom );
	VERIFY( ud );
	return ud->ph_ref_object;

}
#endif