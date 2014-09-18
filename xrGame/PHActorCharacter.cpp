#include "stdafx.h"
#include "phactorcharacter.h"
#include "Extendedgeom.h"
#include "PhysicsCommon.h"
#include "GameObject.h"
#include "PhysicsShellHolder.h"
#include "ai/stalker/ai_stalker.h"
#include "Actor.h"
#include "../xrEngine/gamemtllib.h"
#include "level.h"

//const float JUMP_HIGHT=0.5;
const float JUMP_UP_VELOCITY=6.0f;//5.6f;
const float JUMP_INCREASE_VELOCITY_RATE=1.2f;

CPHActorCharacter::CPHActorCharacter()
{
	SetRestrictionType(CPHCharacter::rtActor);

	//std::fill(m_restrictors_index,m_restrictors_index+CPHCharacter::rtNone,end(m_restrictors));
	//m_restrictors_index[CPHCharacter::rtStalker]		=begin(m_restrictors)+0;
	//m_restrictors_index[CPHCharacter::rtMonsterMedium]	=begin(m_restrictors)+1;
	
	{
		m_restrictors.resize(3);
		m_restrictors[0]=(xr_new<stalker_restrictor>());
		m_restrictors[1]=xr_new<stalker_small_restrictor>();
		m_restrictors[2]=(xr_new<medium_monster_restrictor>());
	}
}

CPHActorCharacter::~CPHActorCharacter(void)
{
	ClearRestrictors();
}
static u16 slide_material_index = GAMEMTL_NONE_IDX;
void CPHActorCharacter::Create(dVector3 sizes)
{
	if(b_exist) return;
	inherited::Create(sizes);
	if(!IsGameTypeSingle())
	{
		ClearRestrictors();
	}
	RESTRICTOR_I i=begin(m_restrictors),e=end(m_restrictors);
	for(;e!=i;++i)
	{
		(*i)->Create(this,sizes);
	}

	if(m_phys_ref_object)
	{
		SetPhysicsRefObject(m_phys_ref_object);
	}
	if(slide_material_index == GAMEMTL_NONE_IDX)
	{
		GameMtlIt mi = GMLib.GetMaterialIt("materials\\earth_slide");
		if( mi != GMLib.LastMaterial	())
			slide_material_index =u16( mi - GMLib.FirstMaterial() );
		//slide_material_index = GMLib.GetMaterialIdx("earth_slide");
	}
}
void	CPHActorCharacter::	ValidateWalkOn						()
{
	
	if( LastMaterialIDX( ) ==  slide_material_index )
		b_clamb_jump = false;
	else
		inherited::ValidateWalkOn();
}
void SPHCharacterRestrictor::Create(CPHCharacter* ch,dVector3 sizes)
{
	VERIFY(ch);
	if(m_character)return;
	m_character=ch;
	m_restrictor=dCreateCylinder(0,m_restrictor_radius,sizes[1]);
	dGeomSetPosition(m_restrictor,0.f,sizes[1]/2.f,0.f);
	m_restrictor_transform=dCreateGeomTransform(0);
	dGeomTransformSetCleanup(m_restrictor_transform,0);
	dGeomTransformSetInfo(m_restrictor_transform,1);
	dGeomTransformSetGeom(m_restrictor_transform,m_restrictor);
	dGeomCreateUserData(m_restrictor);
	dGeomGetUserData(m_restrictor)->b_static_colide=false;
	
	dGeomSetBody(m_restrictor_transform,m_character->get_body());
	dSpaceAdd(m_character->dSpace(),m_restrictor_transform);
	dGeomUserDataSetPhObject(m_restrictor,(CPHObject*)m_character);
	switch(m_type) {
		case CPHCharacter::rtStalker:static_cast<CPHActorCharacter::stalker_restrictor*>(this)->Create(ch,sizes);
		break;
		case CPHCharacter::rtStalkerSmall:static_cast<CPHActorCharacter::stalker_small_restrictor*>(this)->Create(ch,sizes);
		break;
		case CPHCharacter::rtMonsterMedium:static_cast<CPHActorCharacter::medium_monster_restrictor*>(this)->Create(ch,sizes);
		break;
		default:NODEFAULT;
	}
	
}

RESTRICTOR_I CPHActorCharacter::Restrictor(CPHCharacter::ERestrictionType rtype)
{
	R_ASSERT2(rtype<rtActor,"not valide restrictor");
	return begin(m_restrictors)+rtype;
}
void CPHActorCharacter::SetRestrictorRadius(CPHCharacter::ERestrictionType rtype,float r)
{
	if(m_restrictors.size()>0)(*Restrictor(rtype))->SetRadius(r);
}

void SPHCharacterRestrictor::SetRadius(float r)
{
	m_restrictor_radius=r;
	if(m_character)
	{
		float h;
		dGeomCylinderGetParams(m_restrictor,&r,&h);
		dGeomCylinderSetParams(m_restrictor,m_restrictor_radius,h);
	}
}
void CPHActorCharacter::Destroy()
{
	if(!b_exist) return;
	RESTRICTOR_I i=begin(m_restrictors),e=end(m_restrictors);
	for(;e!=i;++i)
	{
		(*i)->Destroy();
	}
	inherited::Destroy();
}
void CPHActorCharacter::ClearRestrictors()
{
	RESTRICTOR_I i=begin(m_restrictors),e=end(m_restrictors);
	for(;e!=i;++i)
	{
		(*i)->Destroy();
		xr_delete(*i);
	}
	m_restrictors.clear();
}
void SPHCharacterRestrictor::Destroy()
{
	if(m_restrictor) {
		dGeomDestroyUserData(m_restrictor);
		dGeomDestroy(m_restrictor);
		m_restrictor=NULL;
	}

	if(m_restrictor_transform){
		dGeomDestroyUserData(m_restrictor_transform);
		m_restrictor_transform=NULL;
	}
	m_character=NULL;
}
void CPHActorCharacter::SetPhysicsRefObject(CPhysicsShellHolder* ref_object)
{
	inherited::SetPhysicsRefObject(ref_object);
	RESTRICTOR_I i=begin(m_restrictors),e=end(m_restrictors);
	for(;e!=i;++i)
	{
		(*i)->SetPhysicsRefObject(ref_object);
	}
}
void SPHCharacterRestrictor::SetPhysicsRefObject(CPhysicsShellHolder* ref_object)
{
	if(m_character)
		dGeomUserDataSetPhysicsRefObject(m_restrictor,ref_object);
}
void CPHActorCharacter::SetMaterial							(u16 material)
{
	inherited::SetMaterial(material);
	if(!b_exist) return;
	RESTRICTOR_I i=begin(m_restrictors),e=end(m_restrictors);
	for(;e!=i;++i)
	{
		(*i)->SetMaterial(material);
	}
}
void SPHCharacterRestrictor::SetMaterial(u16 material)
{
	dGeomGetUserData(m_restrictor)->material=material;
}
void CPHActorCharacter::SetAcceleration(Fvector accel)
{
	Fvector cur_a,input_a;float cur_mug,input_mug;
	cur_a.set(m_acceleration);cur_mug=m_acceleration.magnitude();
	if(!fis_zero(cur_mug))cur_a.mul(1.f/cur_mug);
	input_a.set(accel);input_mug=accel.magnitude();
	if(!fis_zero(input_mug))input_a.mul(1.f/input_mug);
	if(!cur_a.similar(input_a,0.05f)||!fis_zero(input_mug-cur_mug,0.5f))
						inherited::SetAcceleration(accel);
}
bool	CPHActorCharacter::	CanJump								()
{
	return  !b_lose_control														&& 
			LastMaterialIDX( ) !=  slide_material_index							&& 
			(m_ground_contact_normal[1]>0.5f
			||
			m_elevator_state.ClimbingState());
}
void CPHActorCharacter::Jump(const Fvector& accel)
{
	if(!b_exist) return;
	if(CanJump())
	{
		b_jump=true;
		const dReal* vel=dBodyGetLinearVel(m_body);
		dReal amag =m_acceleration.magnitude();
		if(amag<1.f)amag=1.f;
		if(m_elevator_state.ClimbingState())
		{
			m_elevator_state.GetJumpDir(m_acceleration,m_jump_accel);
			m_jump_accel.mul(JUMP_UP_VELOCITY/2.f);
 			//if(accel.square_magnitude()>EPS_L)m_jump_accel.mul(4.f);
		}
		else{
			m_jump_accel.set(vel[0]*JUMP_INCREASE_VELOCITY_RATE+m_acceleration.x/amag*0.2f,jump_up_velocity,vel[2]*JUMP_INCREASE_VELOCITY_RATE +m_acceleration.z/amag*0.2f);
		}
		Enable();
	}
}
void CPHActorCharacter::SetObjectContactCallback(ObjectContactCallbackFun* callback)
{
	inherited::SetObjectContactCallback(callback);

}

void CPHActorCharacter::Disable()
{
	inherited::Disable();
}


struct SFindPredicate
{
	SFindPredicate(const dContact* ac,bool *b)
	{
		c=ac;
		b1=b;
	}
	bool			*b1	;
	const dContact	*c	;
	bool operator ()	(SPHCharacterRestrictor* o)
	{
		*b1=c->geom.g1==o->m_restrictor_transform;
		return *b1||c->geom.g2==o->m_restrictor_transform;
	}
};
void CPHActorCharacter::InitContact(dContact* c,bool &do_collide,u16 material_idx_1,u16	material_idx_2 )
{

	bool b1;
	SFindPredicate fp(c,&b1);
	RESTRICTOR_I r=std::find_if(begin(m_restrictors),end(m_restrictors),fp);
	bool b_restrictor=(r!=end(m_restrictors));
	SGameMtl*	material_1=GMLib.GetMaterialByIdx(material_idx_1);
	SGameMtl*	material_2=GMLib.GetMaterialByIdx(material_idx_2);
	if((material_1&&material_1->Flags.test(SGameMtl::flActorObstacle))||(material_2&&material_2->Flags.test(SGameMtl::flActorObstacle)))
		do_collide=true;
	if(IsGameTypeSingle())
	{
	
		if(b_restrictor)
		{
			b_side_contact=true;
			//MulSprDmp(c->surface.soft_cfm,c->surface.soft_erp,def_spring_rate,def_dumping_rate);
			c->surface.mu		=0.00f;
		}
		else
			inherited::InitContact(c,do_collide,material_idx_1,material_idx_2);
		if(b_restrictor&&
			do_collide&&
			!(b1 ? static_cast<CPHCharacter*>(retrieveGeomUserData(c->geom.g2)->ph_object)->ActorMovable():static_cast<CPHCharacter*>(retrieveGeomUserData(c->geom.g1)->ph_object)->ActorMovable())
			)
		{
			dJointID contact_joint	= dJointCreateContactSpecial(0, ContactGroup, c);
			Enable();
			CPHObject::Island().DActiveIsland()->ConnectJoint(contact_joint);
			if(b1)
				dJointAttach			(contact_joint, dGeomGetBody(c->geom.g1), 0);
			else
				dJointAttach			(contact_joint, 0, dGeomGetBody(c->geom.g2));
			do_collide=false;
			m_friction_factor*=0.1f;
			
		}
	}
	else
	{
		
		dxGeomUserData* D1=retrieveGeomUserData(c->geom.g1);
		dxGeomUserData* D2=retrieveGeomUserData(c->geom.g2);
		if(D1&&D2)
		{
			CActor* A1=smart_cast<CActor*>(D1->ph_ref_object);
			CActor* A2=smart_cast<CActor*>(D2->ph_ref_object);
			if(A1&&A2)
			{
				do_collide=do_collide&&!b_restrictor&&(A1->PPhysicsShell()==0)==(A2->PPhysicsShell()==0);
				c->surface.mu=1.f;
			}
		}
		if(do_collide)inherited::InitContact(c,do_collide,material_idx_1,material_idx_2);
	}
}

void CPHActorCharacter::ChooseRestrictionType	(CPHCharacter::ERestrictionType my_type,float my_depth,CPHCharacter *ch)
{
if (my_type!=rtStalker||(ch->RestrictionType()!=rtStalker&&ch->RestrictionType()!=rtStalkerSmall))return;
float checkR=m_restrictors[rtStalkerSmall]->m_restrictor_radius;//1.5f;//+m_restrictors[rtStalker]->m_restrictor_radius)/2.f;

switch(ch->RestrictionType())
{
case rtStalkerSmall:
	if( ch->ObjectRadius() > checkR )
	{
		//if(my_depth>0.05f)
		ch->SetNewRestrictionType(rtStalker);
		Enable();
		//else ch->SetRestrictionType(rtStalker);
#ifdef DEBUG
		if(ph_dbg_draw_mask1.test(ph_m1_DbgActorRestriction))
				Msg("restriction ready to change small -> large");
#endif
	}
	break;
case rtStalker:
	if( ch->ObjectRadius() < checkR )
	{
#ifdef DEBUG
		if(ph_dbg_draw_mask1.test(ph_m1_DbgActorRestriction))
						Msg("restriction  change large ->  small");
#endif
		ch->SetRestrictionType(rtStalkerSmall);
		Enable();
	}
	break;
default:NODEFAULT;
}

}

void		CPHActorCharacter ::update_last_material()
{
	if( ignore_material( *p_lastMaterialIDX ) )
				inherited::update_last_material();
}