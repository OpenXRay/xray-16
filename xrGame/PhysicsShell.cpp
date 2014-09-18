#include "stdafx.h"
#pragma hdrstop
#include "physicsshell.h"
#include "PHDynamicData.h"
#include "Physics.h"
#include "PHJoint.h"
#include "PHShell.h"
#include "PHJoint.h"
#include "PHJointDestroyInfo.h"
#include "PHSplitedShell.h"
#include "gameobject.h"
#include "physicsshellholder.h"
#include "../Include/xrRender/Kinematics.h"
#include "objectdump.h"
#include "phvalide.h"
extern CPHWorld			*ph_world;
CPhysicsShell::~CPhysicsShell()
{
	
	//if(ph_world)ph_world->NetRelcase(this);
}

CPhysicsElement*			P_create_Element		()
{
	CPHElement* element=xr_new<CPHElement>	();
	return element;
}

CPhysicsShell*				P_create_Shell			()
{
	CPhysicsShell* shell=xr_new<CPHShell>	();
	return shell;
}

CPhysicsShell*				P_create_splited_Shell	()
{
	CPhysicsShell* shell=xr_new<CPHSplitedShell>	();
	return shell;
}

CPhysicsJoint*				P_create_Joint			(CPhysicsJoint::enumType type ,CPhysicsElement* first,CPhysicsElement* second)
{
	CPhysicsJoint* joint=xr_new<CPHJoint>	( type , first, second);
	return joint;
}


CPhysicsShell*	P_build_Shell			(CGameObject* obj,bool not_active_state,BONE_P_MAP* bone_map, bool not_set_bone_callbacks)
{
	VERIFY( obj );
	phys_shell_verify_object_model( *obj );

	IKinematics* pKinematics=smart_cast<IKinematics*>(obj->Visual());

	CPhysicsShell* pPhysicsShell		= P_create_Shell();
#ifdef DEBUG
	pPhysicsShell->dbg_obj=smart_cast<CPhysicsShellHolder*>(obj);
#endif
	pPhysicsShell->build_FromKinematics(pKinematics,bone_map);

	pPhysicsShell->set_PhysicsRefObject(smart_cast<CPhysicsShellHolder*>(obj));
	pPhysicsShell->mXFORM.set(obj->XFORM());
	pPhysicsShell->Activate( not_active_state, not_set_bone_callbacks );//,
	//m_pPhysicsShell->SmoothElementsInertia(0.3f);
	pPhysicsShell->SetAirResistance();//0.0014f,1.5f

	return pPhysicsShell;
}

void	fix_bones(LPCSTR	fixed_bones,CPhysicsShell* shell )
{
		VERIFY(fixed_bones);
		VERIFY(shell);
		IKinematics	*pKinematics = shell->PKinematics();
		VERIFY(pKinematics);
		int count =					_GetItemCount(fixed_bones);
		for (int i=0 ;i<count; ++i) 
		{
			string64					fixed_bone							;
			_GetItem					(fixed_bones,i,fixed_bone)			;
			u16 fixed_bone_id=pKinematics->LL_BoneID(fixed_bone)			;
			R_ASSERT2(BI_NONE!=fixed_bone_id,"wrong fixed bone")			;
			CPhysicsElement* E = shell->get_Element(fixed_bone_id)			;
			if(E)
				E->Fix();
		}
}
CPhysicsShell*				P_build_Shell			(CGameObject* obj,bool not_active_state,BONE_P_MAP* p_bone_map,LPCSTR	fixed_bones)
{
	CPhysicsShell* pPhysicsShell;
	IKinematics* pKinematics=smart_cast<IKinematics*>(obj->Visual());
	if(fixed_bones)
	{


		int count =					_GetItemCount(fixed_bones);
		for (int i=0 ;i<count; ++i) 
		{
			string64					fixed_bone							;
			_GetItem					(fixed_bones,i,fixed_bone)			;
			u16 fixed_bone_id=pKinematics->LL_BoneID(fixed_bone)			;
			R_ASSERT2(BI_NONE!=fixed_bone_id,"wrong fixed bone")			;
			p_bone_map->insert(mk_pair(fixed_bone_id,physicsBone()))			;
		}

		pPhysicsShell=P_build_Shell(obj,not_active_state,p_bone_map);

		//m_pPhysicsShell->add_Joint(P_create_Joint(CPhysicsJoint::enumType::full_control,0,fixed_element));
	}
	else
		pPhysicsShell=P_build_Shell(obj,not_active_state);


	BONE_P_PAIR_IT i=p_bone_map->begin(),e=p_bone_map->end();
	if(i!=e) pPhysicsShell->SetPrefereExactIntegration();
	for(;i!=e;i++)
	{
		CPhysicsElement* fixed_element=i->second.element;
		R_ASSERT2(fixed_element,"fixed bone has no physics");
		//if(!fixed_element) continue;
		fixed_element->Fix();
	}
	return pPhysicsShell;
}

CPhysicsShell*				P_build_Shell			(CGameObject* obj,bool not_active_state,LPCSTR	fixed_bones)
{
	U16Vec f_bones;
	if(fixed_bones){
		IKinematics* K		= smart_cast<IKinematics*>(obj->Visual());
		int count =			_GetItemCount(fixed_bones);
		for (int i=0 ;i<count; ++i){
			string64		fixed_bone;
			_GetItem		(fixed_bones,i,fixed_bone);
			f_bones.push_back(K->LL_BoneID(fixed_bone));
			R_ASSERT2(BI_NONE!=f_bones.back(),"wrong fixed bone")			;
		}
	}
	return P_build_Shell	(obj,not_active_state,f_bones);
}

static BONE_P_MAP bone_map=BONE_P_MAP();
CPhysicsShell*				P_build_Shell			(CGameObject* obj,bool not_active_state,U16Vec& fixed_bones)
{
	bone_map.clear			();
	CPhysicsShell*			pPhysicsShell;
	if(!fixed_bones.empty())
		for (U16It it=fixed_bones.begin(); it!=fixed_bones.end(); it++)
			bone_map.insert(mk_pair(*it,physicsBone()));
	pPhysicsShell=P_build_Shell(obj,not_active_state,&bone_map);

	// fix bones
	BONE_P_PAIR_IT i=bone_map.begin(),e=bone_map.end();
	if(i!=e) pPhysicsShell->SetPrefereExactIntegration();
	for(;i!=e;i++){
		CPhysicsElement* fixed_element=i->second.element;
		//R_ASSERT2(fixed_element,"fixed bone has no physics");
		if(!fixed_element) continue;
		fixed_element->Fix();
	}
	return pPhysicsShell;
}

CPhysicsShell*	P_build_SimpleShell(CGameObject* obj,float mass,bool not_active_state)
{
	CPhysicsShell* pPhysicsShell		= P_create_Shell();
#ifdef DEBUG
	pPhysicsShell->dbg_obj=smart_cast<CPhysicsShellHolder*>(obj);
#endif
	Fobb obb; obj->Visual()->getVisData().box.get_CD(obb.m_translate,obb.m_halfsize); obb.m_rotate.identity();
	CPhysicsElement* E = P_create_Element(); R_ASSERT(E); E->add_Box(obb);
	pPhysicsShell->add_Element(E);
	pPhysicsShell->setMass(mass);
	pPhysicsShell->set_PhysicsRefObject(smart_cast<CPhysicsShellHolder*>(obj));
	if(!obj->H_Parent())
		pPhysicsShell->Activate(obj->XFORM(),0,obj->XFORM(),not_active_state);
	return pPhysicsShell;
}

void ApplySpawnIniToPhysicShell(CInifile* ini,CPhysicsShell* physics_shell,bool fixed)
{
		if(!ini)
			return;
		if(ini->section_exist("physics_common"))
		{
			fixed = fixed || (ini->line_exist("physics_common","fixed_bones")) ;
#pragma todo("not ignore static if non realy fixed! ")
			fix_bones(ini->r_string("physics_common","fixed_bones"),physics_shell);
		}
		if(ini->section_exist("collide"))
		{

			if((ini->line_exist("collide","ignore_static")&&fixed)||(ini->line_exist("collide","ignore_static")&&ini->section_exist("animated_object")))
			{
				physics_shell->SetIgnoreStatic();
			}
			if(ini->line_exist("collide","small_object"))
			{
				physics_shell->SetSmall();
			}
			if(ini->line_exist("collide","ignore_small_objects"))
			{
				physics_shell->SetIgnoreSmall();
			}
			if(ini->line_exist("collide","ignore_ragdoll"))
			{
				physics_shell->SetIgnoreRagDoll();
			}


			//If need, then show here that it is needed to ignore collisions with "animated_object"
			if (ini->line_exist("collide","ignore_animated_objects"))
			{
				physics_shell->SetIgnoreAnimated();
			}

		}
		//If next section is available then given "PhysicShell" is classified
		//as animated and we read options for his animation
		
		if (ini->section_exist("animated_object"))
		{
			//Show that given "PhysicShell" animated
			physics_shell->CreateShellAnimator( ini, "animated_object" );
		}
	
}



void	get_box( const CPhysicsBase*	shell, const	Fmatrix& form,	Fvector&	sz, Fvector&	c )
{
	t_get_box( shell, form, sz, c );
}





void destroy_physics_shell(CPhysicsShell* &p)
{
	if (p)
		p->Deactivate();
	xr_delete(p);
}

bool bone_has_pysics( IKinematics& K, u16 bone_id )
{
	return K.LL_GetBoneVisible( bone_id ) && shape_is_physic(K.LL_GetData( bone_id ).shape);
}

bool has_physics_collision_shapes( IKinematics& K )
{
	u16 nbb = K.LL_BoneCount();
	for(u16 i = 0; i < nbb; ++i )
		if( bone_has_pysics( K, i ) )
			return true;
	return false;
}

void	phys_shell_verify_model( IKinematics& K )
{
	IRenderVisual* V = K.dcast_RenderVisual();
	VERIFY( V );
	VERIFY2( has_physics_collision_shapes( K ), make_string( "Can not create physics shell for model %s because it has no physics collision shapes set", V->getDebugName().c_str() ) );
}

void	phys_shell_verify_object_model( CObject& O )	
{
	IRenderVisual	*V = O.Visual();
	VERIFY2( V, make_string( "Can not create physics shell for object %s it has no model", O.cName().c_str() )+ make_string("\n object dump: \n") + dbg_object_full_dump_string( &O ) );
	IKinematics		*K = V->dcast_PKinematics();
	VERIFY2( K, make_string( "Can not create physics shell for object %s, model %s is not skeleton", O.cName().c_str(), O.cNameVisual().c_str() ) );
	VERIFY2( has_physics_collision_shapes( *K ), make_string( "Can not create physics shell for object %s, model %s has no physics collision shapes set", O.cName().c_str(), O.cNameVisual().c_str() )+ make_string("\n object dump: \n") + dbg_object_full_dump_string( &O )  );
	VERIFY2( _valid( O.XFORM() ), make_string( "create physics shell: object matrix is not valide" ) + make_string("\n object dump: \n") + dbg_object_full_dump_string( &O ) );
	VERIFY2(valid_pos( O.XFORM().c ),  dbg_valide_pos_string( O.XFORM().c, &O, "create physics shell" ) );
}