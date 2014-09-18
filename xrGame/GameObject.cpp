#include "pch_script.h"
#include "GameObject.h"
//#include "../Include/xrRender/RenderVisual.h"
#include "../Include/xrRender/RenderVisual.h"
#include "../xrphysics/PhysicsShell.h"
#include "ai_space.h"
#include "CustomMonster.h" 
#include "physicobject.h"
#include "HangingLamp.h"
#include "../xrphysics/PhysicsShell.h"
#include "game_sv_single.h"
#include "level_graph.h"
#include "ph_shell_interface.h"
#include "script_game_object.h"
#include "xrserver_objects_alife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "game_cl_base.h"
#include "object_factory.h"
#include "../Include/xrRender/Kinematics.h"
#include "ai_object_location_impl.h"
#include "game_graph.h"
#include "ai_debug.h"
#include "../xrEngine/igame_level.h"
#include "level.h"
#include "script_callback_ex.h"
#include "../xrphysics/MathUtils.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "game_cl_mp.h"
#include "reward_event_generator.h"
#include "game_level_cross_table.h"
#include "ai_obstacle.h"
#include "magic_box3.h"
#include "animation_movement_controller.h"
#include "../xrengine/xr_collide_form.h"
extern MagicBox3 MagicMinBox (int iQuantity, const Fvector* akPoint);

#pragma warning(push)
#pragma warning(disable:4995)
#include <malloc.h>
#pragma warning(pop)

#ifdef DEBUG
#	include "debug_renderer.h"
#	include "PHDebug.h"
#endif

ENGINE_API bool g_dedicated_server;

CGameObject::CGameObject		()
{
	m_ai_obstacle				= 0;

	init						();
	//-----------------------------------------
	m_bCrPr_Activated			= false;
	m_dwCrPr_ActivationStep		= 0;
	m_spawn_time				= 0;
	m_ai_location				= !g_dedicated_server ? xr_new<CAI_ObjectLocation>() : 0;
	m_server_flags.one			();

	m_callbacks					= xr_new<CALLBACK_MAP>();
	m_anim_mov_ctrl				= 0;
}

CGameObject::~CGameObject		()
{
	VERIFY						( !animation_movement( ) );
	VERIFY						(!m_ini_file);
	VERIFY						(!m_lua_game_object);
	VERIFY						(!m_spawned);
	xr_delete					(m_ai_location);
	xr_delete					(m_callbacks);
	xr_delete					(m_ai_obstacle);
}

void CGameObject::init			()
{
	m_lua_game_object			= 0;
	m_script_clsid				= -1;
	m_ini_file					= 0;
	m_spawned					= false;
}

void CGameObject::Load(LPCSTR section)
{
	inherited::Load			(section);
	ISpatial*		self				= smart_cast<ISpatial*> (this);
	if (self)	{
		// #pragma todo("to Dima: All objects are visible for AI ???")
		// self->spatial.type	|=	STYPE_VISIBLEFORAI;	
		self->spatial.type	&= ~STYPE_REACTTOSOUND;
	}
}

void CGameObject::reinit	()
{
	m_visual_callback.clear	();
	if (!g_dedicated_server)
        ai_location().reinit	();

	// clear callbacks	
	for (CALLBACK_MAP_IT it = m_callbacks->begin(); it != m_callbacks->end(); ++it) it->second.clear();
}

void CGameObject::reload	(LPCSTR section)
{
	m_script_clsid				= object_factory().script_clsid(CLS_ID);
}

void CGameObject::net_Destroy	()
{
#ifdef DEBUG
	if (psAI_Flags.test(aiDestroy))
		Msg					("Destroying client object [%d][%s][%x]",ID(),*cName(),this);
#endif

	VERIFY					(m_spawned);
	if( m_anim_mov_ctrl )
					destroy_anim_mov_ctrl	();

	xr_delete				(m_ini_file);

	m_script_clsid			= -1;
	if (Visual() && smart_cast<IKinematics*>(Visual()))
		smart_cast<IKinematics*>(Visual())->Callback	(0,0);

	inherited::net_Destroy						();
	setReady									(FALSE);
	
	if (Level().IsDemoPlayStarted() && ID() == u16(-1))
	{
		Msg("Destroying demo_spectator object");
	} else
	{
		g_pGameLevel->Objects.net_Unregister		(this);
	}
	
	if (this == Level().CurrentEntity())
	{
		if (!Level().IsDemoPlayStarted())
		{
			Level().SetControlEntity			(0);
		}
		Level().SetEntity						(0);	// do not switch !!!
	}

	Level().RemoveObject_From_4CrPr(this);

//.	Parent									= 0;

	CScriptBinder::net_Destroy				();

	xr_delete								(m_lua_game_object);
	m_spawned								= false;
}

void CGameObject::OnEvent		(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_HIT:
	case GE_HIT_STATISTIC:
		{
/*
			u16				id,weapon_id;
			Fvector			dir;
			float			power, impulse;
			s16				element;
			Fvector			position_in_bone_space;
			u16				hit_type;
			float			ap = 0.0f;

			P.r_u16			(id);
			P.r_u16			(weapon_id);
			P.r_dir			(dir);
			P.r_float		(power);
			P.r_s16			(element);
			P.r_vec3		(position_in_bone_space);
			P.r_float		(impulse);
			P.r_u16			(hit_type);	//hit type
			if ((ALife::EHitType)hit_type == ALife::eHitTypeFireWound)
			{
				P.r_float	(ap);
			}

			CObject*	Hitter = Level().Objects.net_Find(id);
			CObject*	Weapon = Level().Objects.net_Find(weapon_id);

			SHit	HDS = SHit(power, dir, Hitter, element, position_in_bone_space, impulse, (ALife::EHitType)hit_type, ap);
*/
			SHit	HDS;
			HDS.PACKET_TYPE = type;
			HDS.Read_Packet_Cont(P);
//			Msg("Hit received: %d[%d,%d]", HDS.whoID, HDS.weaponID, HDS.BulletID);
			CObject*	Hitter = Level().Objects.net_Find(HDS.whoID);
			CObject*	Weapon = Level().Objects.net_Find(HDS.weaponID);
			HDS.who		= Hitter;
			if (!HDS.who)
			{
				Msg("! ERROR: hitter object [%d] is NULL on client.", HDS.whoID);
			}
			//-------------------------------------------------------
			switch (HDS.PACKET_TYPE)
			{
			case GE_HIT_STATISTIC:
				{
					if (GameID() != eGameIDSingle)
						Game().m_WeaponUsageStatistic->OnBullet_Check_Request(&HDS);
				}break;
			default:
				{
				}break;
			}
			SetHitInfo(Hitter, Weapon, HDS.bone(), HDS.p_in_bone_space, HDS.dir);
			Hit				(&HDS);
			//---------------------------------------------------------------------------
			if (GameID() != eGameIDSingle)
			{
				Game().m_WeaponUsageStatistic->OnBullet_Check_Result(false);
				game_cl_mp*	mp_game = smart_cast<game_cl_mp*>(&Game());
				if (mp_game->get_reward_generator())
					mp_game->get_reward_generator()->OnBullet_Hit(Hitter, this, Weapon, HDS.boneID);
			}
			//---------------------------------------------------------------------------
		}
		break;
	case GE_DESTROY:
		{
			if ( H_Parent() )
			{
				Msg( "! ERROR (GameObject): GE_DESTROY arrived to object[%d][%s], that has parent[%d][%s], frame[%d]",
					ID(), cNameSect().c_str(),
					H_Parent()->ID(), H_Parent()->cName().c_str(), Device.dwFrame );
				
				// This object will be destroy on call function <H_Parent::Destroy>
				// or it will be call <H_Parent::Reject>  ==>  H_Parent = NULL
				// !!! ___ it is necessary to be check!
				break;
			}
#ifdef MP_LOGGING
			Msg("--- Object: GE_DESTROY of [%d][%s]", ID(), cNameSect().c_str());
#endif // MP_LOGGING

			setDestroy		(TRUE);
//			MakeMeCrow		();
		}
		break;
	}
}

void VisualCallback(IKinematics *tpKinematics);

BOOL CGameObject::net_Spawn		(CSE_Abstract*	DC)
{
	VERIFY							(!m_spawned);
	m_spawned						= true;
	m_spawn_time					= Device.dwFrame;
	m_ai_obstacle					= xr_new<ai_obstacle>(this);

	CSE_Abstract					*E = (CSE_Abstract*)DC;
	VERIFY							(E);

	const CSE_Visual				*visual	= smart_cast<const CSE_Visual*>(E);
	if (visual) {
		cNameVisual_set				(visual_name(E));
		if (visual->flags.test(CSE_Visual::flObstacle)) {
			ISpatial				*self = smart_cast<ISpatial*>(this);
			self->spatial.type		|=	STYPE_OBSTACLE;
		}
	}

	// Naming
	cName_set						(E->s_name);
	cNameSect_set					(E->s_name);
	if (E->name_replace()[0])
		cName_set					(E->name_replace());
	bool demo_spectator = false;
	
	if (Level().IsDemoPlayStarted() && E->ID == u16(-1))
	{
		Msg("* Spawning demo spectator ...");
		demo_spectator = true;
	} else {
		R_ASSERT(Level().Objects.net_Find(E->ID) == NULL);
	}


	setID							(E->ID);
//	if (GameID() != eGameIDSingle)
//		Msg ("CGameObject::net_Spawn -- object %s[%x] setID [%d]", *(E->s_name), this, E->ID);
	
	// XForm
	XFORM().setXYZ					(E->o_Angle);
	Position().set					(E->o_Position);
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Spawn obj %s Position set from CSE_Abstract %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif
	VERIFY							(_valid(renderable.xform));
	VERIFY							(!fis_zero(DET(renderable.xform)));
	CSE_ALifeObject					*O = smart_cast<CSE_ALifeObject*>(E);
	if (O && xr_strlen(O->m_ini_string)) {
#pragma warning(push)
#pragma warning(disable:4238)
		m_ini_file					= xr_new<CInifile>(
			&IReader				(
				(void*)(*(O->m_ini_string)),
				O->m_ini_string.size()
			),
			FS.get_path("$game_config$")->m_Path
		);
#pragma warning(pop)
	}

	m_story_id						= ALife::_STORY_ID(-1);
	if (O)
		m_story_id					= O->m_story_id;

	// Net params
	setLocal						(E->s_flags.is(M_SPAWN_OBJECT_LOCAL));
	if (Level().IsDemoPlay()) //&& OnClient())
	{
		if (!demo_spectator)
		{
			setLocal(FALSE);
		}
	};

	setReady						(TRUE);
	if (!demo_spectator)
		g_pGameLevel->Objects.net_Register	(this);

	m_server_flags.one				();
	if (O) {
		m_server_flags					= O->m_flags;
		if (O->m_flags.is(CSE_ALifeObject::flVisibleForAI))
			spatial.type				|= STYPE_VISIBLEFORAI;
		else
			spatial.type				= (spatial.type | STYPE_VISIBLEFORAI) ^ STYPE_VISIBLEFORAI;
	}

	reload						(*cNameSect());
	if(!g_dedicated_server)
		CScriptBinder::reload	(*cNameSect());
	
	reinit						();
	if(!g_dedicated_server)
		CScriptBinder::reinit	();
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Spawn obj %s After Script Binder reinit %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
#endif
	//load custom user data from server
	if(!E->client_data.empty())
	{	
//		Msg				("client data is present for object [%d][%s], load is processed",ID(),*cName());
		IReader			ireader = IReader(&*E->client_data.begin(), E->client_data.size());
		net_Load		(ireader);
	}
	else {
//		Msg				("no client data for object [%d][%s], load is skipped",ID(),*cName());
	}

	// if we have a parent
	if ( ai().get_level_graph() ) {
		if ( E->ID_Parent == 0xffff ) {
			CSE_ALifeObject* l_tpALifeObject	= smart_cast<CSE_ALifeObject*>(E);
			if (l_tpALifeObject && ai().level_graph().valid_vertex_id(l_tpALifeObject->m_tNodeID))
				ai_location().level_vertex		(l_tpALifeObject->m_tNodeID);
			else {
				CSE_Temporary* l_tpTemporary	= smart_cast<CSE_Temporary*>	(E);
				if (l_tpTemporary && ai().level_graph().valid_vertex_id(l_tpTemporary->m_tNodeID))
					ai_location().level_vertex	(l_tpTemporary->m_tNodeID);
			}

			if (l_tpALifeObject && ai().game_graph().valid_vertex_id(l_tpALifeObject->m_tGraphID))
				ai_location().game_vertex		(l_tpALifeObject->m_tGraphID);

			validate_ai_locations				(false);

			// validating position
			if	(
					UsedAI_Locations() && 
					ai().level_graph().inside(
						ai_location().level_vertex_id(),
						Position()
					) &&
					can_validate_position_on_spawn()
				)
				Position().y					= EPS_L + ai().level_graph().vertex_plane_y(*ai_location().level_vertex(),Position().x,Position().z);
		}
		else {
			CSE_ALifeObject* const alife_object	= smart_cast<CSE_ALifeObject*>(E);
			if ( alife_object && ai().level_graph().valid_vertex_id(alife_object->m_tNodeID) ) {
				ai_location().level_vertex		(alife_object->m_tNodeID);
				ai_location().game_vertex		(alife_object->m_tGraphID);
			}
		}
	}
	inherited::net_Spawn		(DC);

	m_bObjectRemoved			= false;

	spawn_supplies				();
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Spawn obj %s Before CScriptBinder::net_Spawn %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
	BOOL ret =CScriptBinder::net_Spawn(DC);
#else
	return						(CScriptBinder::net_Spawn(DC));
#endif

#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Spawn obj %s Before CScriptBinder::net_Spawn %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}
	return ret;
#endif
}

void CGameObject::net_Save		(NET_Packet &net_packet)
{
	u32							position;
	net_packet.w_chunk_open16	(position);
	save						(net_packet);

	// Script Binder Save ---------------------------------------
#ifdef DEBUG	
	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> **** Save script object [%s] *****", *cName());
		Msg(">> Before save :: packet position = [%u]", net_packet.w_tell());
	}

#endif

	CScriptBinder::save			(net_packet);

#ifdef DEBUG	

	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> After save :: packet position = [%u]", net_packet.w_tell());
	}
#endif

	// ----------------------------------------------------------

	net_packet.w_chunk_close16	(position);
}

void CGameObject::net_Load		(IReader &ireader)
{
	load					(ireader);

	// Script Binder Load ---------------------------------------
#ifdef DEBUG	
	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> **** Load script object [%s] *****", *cName());
		Msg(">> Before load :: reader position = [%i]", ireader.tell());
	}

#endif

	CScriptBinder::load		(ireader);


#ifdef DEBUG	

	if (psAI_Flags.test(aiSerialize))	{
		Msg(">> After load :: reader position = [%i]", ireader.tell());
	}
#endif
	// ----------------------------------------------------------
#ifdef DEBUG
	if(ph_dbg_draw_mask1.test(ph_m1_DbgTrackObject)&&stricmp(PH_DBG_ObjectTrackName(),*cName())==0)
	{
		Msg("CGameObject::net_Load obj %s (loaded) %f,%f,%f",PH_DBG_ObjectTrackName(),Position().x,Position().y,Position().z);
	}

#endif

}

void CGameObject::save			(NET_Packet &output_packet) 
{
}

void CGameObject::load			(IReader &input_packet)
{
}

void CGameObject::spawn_supplies()
{
	if (!spawn_ini() || ai().get_alife())
		return;

	if (!spawn_ini()->section_exist("spawn"))
		return;

	LPCSTR					N,V;
	float					p;
	bool bScope				=	false;
	bool bSilencer			=	false;
	bool bLauncher			=	false;

	for (u32 k = 0, j; spawn_ini()->r_line("spawn",k,&N,&V); k++) {
		VERIFY				(xr_strlen(N));
		j					= 1;
		p					= 1.f;
		
		float f_cond						= 1.0f;
		if (V && xr_strlen(V)) {
			int				n = _GetItemCount(V);
			string16		temp;
			if (n > 0)
				j			= atoi(_GetItem(V,0,temp)); //count
			
			if(NULL!=strstr(V,"prob="))
				p			=(float)atof(strstr(V,"prob=")+5);
			if (fis_zero(p))p = 1.f;
			if (!j)	j		= 1;
			if(NULL!=strstr(V,"cond="))
				f_cond		= (float)atof(strstr(V,"cond=")+5);
			bScope			=	(NULL!=strstr(V,"scope"));
			bSilencer		=	(NULL!=strstr(V,"silencer"));
			bLauncher		=	(NULL!=strstr(V,"launcher"));

		}
		for (u32 i=0; i<j; ++i)
			if (::Random.randF(1.f) < p){
				CSE_Abstract* A=Level().spawn_item	(N,Position(),ai_location().level_vertex_id(),ID(),true);

				CSE_ALifeInventoryItem*	pSE_InventoryItem = smart_cast<CSE_ALifeInventoryItem*>(A);
				if(pSE_InventoryItem)
						pSE_InventoryItem->m_fCondition = f_cond;

				CSE_ALifeItemWeapon* W =  smart_cast<CSE_ALifeItemWeapon*>(A);
				if (W) {
					if (W->m_scope_status			== ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonScope, bScope);
					if (W->m_silencer_status		== ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonSilencer, bSilencer);
					if (W->m_grenade_launcher_status == ALife::eAddonAttachable)
						W->m_addon_flags.set(CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher, bLauncher);
				}

				NET_Packet					P;
				A->Spawn_Write				(P,TRUE);
				Level().Send				(P,net_flags(TRUE));
				F_entity_Destroy			(A);
		}
	}
}

void CGameObject::setup_parent_ai_locations(bool assign_position)
{
//	CGameObject				*l_tpGameObject	= static_cast<CGameObject*>(H_Root());
	VERIFY					(H_Parent());
	CGameObject				*l_tpGameObject	= static_cast<CGameObject*>(H_Parent());
	VERIFY					(l_tpGameObject);

	// get parent's position
	if ( assign_position && use_parent_ai_locations() )
		Position().set		(l_tpGameObject->Position());

	//if ( assign_position && 
	//		( use_parent_ai_locations() &&
	//		!( cast_attachable_item() && cast_attachable_item()->enabled() )
	//		 ) 
	//	)
	//	Position().set		(l_tpGameObject->Position());

	// setup its ai locations
	if (!UsedAI_Locations())
		return;

	if (!ai().get_level_graph())
		return;

	if (l_tpGameObject->UsedAI_Locations() && ai().level_graph().valid_vertex_id(l_tpGameObject->ai_location().level_vertex_id()))
		ai_location().level_vertex	(l_tpGameObject->ai_location().level_vertex_id());
	else
		validate_ai_locations	(false);
//	VERIFY2						(l_tpGameObject->UsedAI_Locations(),*l_tpGameObject->cNameSect());
//	VERIFY2						(ai().level_graph().valid_vertex_id(l_tpGameObject->ai_location().level_vertex_id()),*cNameSect());
//	ai_location().level_vertex	(l_tpGameObject->ai_location().level_vertex_id());

	if (ai().game_graph().valid_vertex_id(l_tpGameObject->ai_location().game_vertex_id()))
		ai_location().game_vertex	(l_tpGameObject->ai_location().game_vertex_id());
	else
		ai_location().game_vertex	(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());
//	VERIFY2						(ai().game_graph().valid_vertex_id(l_tpGameObject->ai_location().game_vertex_id()),*cNameSect());
//	ai_location().game_vertex	(l_tpGameObject->ai_location().game_vertex_id());
}

u32 CGameObject::new_level_vertex_id			() const
{
	Fvector							center;
	Center							(center);
	center.x						= Position().x;
	center.z						= Position().z;
	return							(ai().level_graph().vertex(ai_location().level_vertex_id(),center));
}

void CGameObject::update_ai_locations			(bool decrement_reference)
{
	u32								l_dwNewLevelVertexID = new_level_vertex_id();
	VERIFY							(ai().level_graph().valid_vertex_id(l_dwNewLevelVertexID));
	if (decrement_reference && (ai_location().level_vertex_id() == l_dwNewLevelVertexID))
		return;

	ai_location().level_vertex		(l_dwNewLevelVertexID);

	if (!ai().get_game_graph() && ai().get_cross_table())
		return;

	ai_location().game_vertex		(ai().cross_table().vertex(ai_location().level_vertex_id()).game_vertex_id());
	VERIFY							(ai().game_graph().valid_vertex_id(ai_location().game_vertex_id()));
}

void CGameObject::validate_ai_locations			(bool decrement_reference)
{
	if (!ai().get_level_graph())
		return;

	if (!UsedAI_Locations())
		return;

	update_ai_locations				(decrement_reference);
}

void CGameObject::spatial_move	()
{
	if (H_Parent())
		setup_parent_ai_locations	();
	else
		if (Visual())
			validate_ai_locations	();

	inherited::spatial_move			();
}

#ifdef DEBUG
void			CGameObject::dbg_DrawSkeleton	()
{
	CCF_Skeleton* Skeleton = smart_cast<CCF_Skeleton*>(collidable.model);
	if (!Skeleton) return;
	Skeleton->_dbg_refresh();

	const CCF_Skeleton::ElementVec& Elements = Skeleton->_GetElements();
	for (CCF_Skeleton::ElementVec::const_iterator I=Elements.begin(); I!=Elements.end(); I++){
		if (!I->valid())		continue;
		switch (I->type){
			case SBoneShape::stBox:{
				Fmatrix M;
				M.invert			(I->b_IM);
				Fvector h_size		= I->b_hsize;
				Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 255, 0, 255));
								   }break;
			case SBoneShape::stCylinder:{
				Fmatrix M;
				M.c.set				(I->c_cylinder.m_center);
				M.k.set				(I->c_cylinder.m_direction);
				Fvector				h_size;
				h_size.set			(I->c_cylinder.m_radius,I->c_cylinder.m_radius,I->c_cylinder.m_height*0.5f);
				Fvector::generate_orthonormal_basis(M.k,M.j,M.i);
				Level().debug_renderer().draw_obb	(M, h_size, color_rgba(0, 127, 255, 255));
										}break;
			case SBoneShape::stSphere:{
				Fmatrix				l_ball;
				l_ball.scale		(I->s_sphere.R, I->s_sphere.R, I->s_sphere.R);
				l_ball.translate_add(I->s_sphere.P);
				Level().debug_renderer().draw_ellipse(l_ball, color_rgba(0, 255, 0, 255));
									  }break;
		};
	};	
}
#endif

void CGameObject::renderable_Render	()
{
	inherited::renderable_Render();
	::Render->set_Transform		(&XFORM());
	::Render->add_Visual		(Visual());
	Visual()->getVisData().hom_frame = Device.dwFrame;
}

/*
float CGameObject::renderable_Ambient	()
{
	return (ai().get_level_graph() && ai().level_graph().valid_vertex_id(level_vertex_id()) ? float(level_vertex()->light()/15.f) : 1.f);
}
*/

CObject::SavedPosition CGameObject::ps_Element(u32 ID) const
{
	VERIFY(ID<ps_Size());
	inherited::SavedPosition	SP	=	PositionStack[ID];
	SP.dwTime					+=	Level().timeServer_Delta();
	return SP;
}

void CGameObject::u_EventGen(NET_Packet& P, u32 type, u32 dest)
{
	P.w_begin	(M_EVENT);
	P.w_u32		(Level().timeServer());
	P.w_u16		(u16(type&0xffff));
	P.w_u16		(u16(dest&0xffff));
}

void CGameObject::u_EventSend(NET_Packet& P, u32 dwFlags )
{
	Level().Send(P, dwFlags);
}

#include "bolt.h"
void CGameObject::OnH_B_Chield()
{
	inherited::OnH_B_Chield();
	///PHSetPushOut();????
}

void CGameObject::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	setup_parent_ai_locations	(false);

	CGameObject					*parent = smart_cast<CGameObject*>(H_Parent());
	VERIFY						(parent);
	if (ai().get_level_graph() && ai().level_graph().valid_vertex_id(parent->ai_location().level_vertex_id()))
		validate_ai_locations	(false);
}

BOOL CGameObject::UsedAI_Locations()
{
	return					(m_server_flags.test(CSE_ALifeObject::flUsedAI_Locations));
}

BOOL CGameObject::TestServerFlag(u32 Flag) const
{
	return					(m_server_flags.test(Flag));
}

void CGameObject::add_visual_callback		(visual_callback *callback)
{
	VERIFY						(smart_cast<IKinematics*>(Visual()));
	CALLBACK_VECTOR_IT			I = std::find(visual_callbacks().begin(),visual_callbacks().end(),callback);
	VERIFY						(I == visual_callbacks().end());

	if (m_visual_callback.empty())	SetKinematicsCallback(true);
//		smart_cast<IKinematics*>(Visual())->Callback(VisualCallback,this);
	m_visual_callback.push_back	(callback);
}

void CGameObject::remove_visual_callback	(visual_callback *callback)
{
	CALLBACK_VECTOR_IT			I = std::find(m_visual_callback.begin(),m_visual_callback.end(),callback);
	VERIFY						(I != m_visual_callback.end());
	m_visual_callback.erase		(I);
	if (m_visual_callback.empty())	SetKinematicsCallback(false);
//		smart_cast<IKinematics*>(Visual())->Callback(0,0);
}

void CGameObject::SetKinematicsCallback		(bool set)
{
	if(!Visual())	return;
	if (set)
		smart_cast<IKinematics*>(Visual())->Callback(VisualCallback,this);
	else
		smart_cast<IKinematics*>(Visual())->Callback(0,0);
};

void VisualCallback	(IKinematics *tpKinematics)
{
	CGameObject						*game_object = static_cast<CGameObject*>(static_cast<CObject*>(tpKinematics->GetUpdateCallbackParam()));
	VERIFY							(game_object);
	
	CGameObject::CALLBACK_VECTOR_IT	I = game_object->visual_callbacks().begin();
	CGameObject::CALLBACK_VECTOR_IT	E = game_object->visual_callbacks().end();
	for ( ; I != E; ++I)
		(*I)						(tpKinematics);
}

CScriptGameObject *CGameObject::lua_game_object		() const
{
#ifdef DEBUG
	if (!m_spawned)
		Msg							("! you are trying to use a destroyed object [%x]",this);
#endif
	THROW							(m_spawned);
	if (!m_lua_game_object)
		m_lua_game_object			= xr_new<CScriptGameObject>(const_cast<CGameObject*>(this));
	return							(m_lua_game_object);
}

bool CGameObject::NeedToDestroyObject()	const
{
	return false;
}

void CGameObject::DestroyObject()			
{
	
	if(m_bObjectRemoved)	return;
	m_bObjectRemoved		= true;
	if (getDestroy())		return;

	if (Local())
	{	
		NET_Packet		P;
		u_EventGen		(P,GE_DESTROY,ID());
		u_EventSend		(P);
	}
}

void CGameObject::shedule_Update	(u32 dt)
{
	//уничтожить
	if(NeedToDestroyObject())
	{
#ifndef MASTER_GOLD
		Msg("--NeedToDestroyObject for [%d][%d]", ID(), Device.dwFrame);
#endif // #ifndef MASTER_GOLD
		DestroyObject			();
	}

	// Msg							("-SUB-:[%x][%s] CGameObject::shedule_Update",smart_cast<void*>(this),*cName());
	inherited::shedule_Update	(dt);
	
	if(!g_dedicated_server)
		CScriptBinder::shedule_Update(dt);
}

BOOL CGameObject::net_SaveRelevant	()
{
	return	(CScriptBinder::net_SaveRelevant());
}

//игровое имя объекта
LPCSTR CGameObject::Name () const
{
	return	(*cName());
}

u32	CGameObject::ef_creature_type		() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid creature type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
}

u32	CGameObject::ef_equipment_type		() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid equipment type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
//	return		(6);
}

u32	CGameObject::ef_main_weapon_type	() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid main weapon type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
//	return		(5);
}

u32	CGameObject::ef_anomaly_type		() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid anomaly type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
}

u32	CGameObject::ef_weapon_type			() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid weapon type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
//	return		(u32(0));
}

u32	CGameObject::ef_detector_type		() const
{
	string16	temp; CLSID2TEXT(CLS_ID,temp);
	R_ASSERT3	(false,"Invalid detector type request, virtual function is not properly overridden!",temp);
	return		(u32(-1));
}

void CGameObject::net_Relcase			(CObject* O)
{
	inherited::net_Relcase		(O);
	if(!g_dedicated_server)
		CScriptBinder::net_Relcase	(O);
}

CGameObject::CScriptCallbackExVoid &CGameObject::callback(GameObject::ECallbackType type) const
{
	return ((*m_callbacks)[type]);
}

LPCSTR CGameObject::visual_name		(CSE_Abstract *server_entity)
{
	const CSE_Visual			*visual	= smart_cast<const CSE_Visual*>(server_entity);
	VERIFY						(visual);
	return						(visual->get_visual());
}		
bool		CGameObject::	animation_movement_controlled	( ) const	
{ 
	return	!!animation_movement() && animation_movement()->IsActive();
}

void CGameObject::update_animation_movement_controller	()
{
	if (!m_anim_mov_ctrl )
		return;

	if (m_anim_mov_ctrl->IsActive())
	{
		m_anim_mov_ctrl->OnFrame	();
		return;
	}

	destroy_anim_mov_ctrl		();
}

bool CGameObject::is_ai_obstacle	() const
{
	return							(false);
}

void	CGameObject::OnChangeVisual	( )
{
	inherited::OnChangeVisual( );
	if ( m_anim_mov_ctrl )
		destroy_anim_mov_ctrl( );
}

bool CGameObject::shedule_Needed( )
{
	return						(!getDestroy());
}

void CGameObject::create_anim_mov_ctrl	( CBlend *b, Fmatrix *start_pose, bool local_animation )
{
	if( animation_movement_controlled( ) )
	{
		m_anim_mov_ctrl->NewBlend	(
			b,
			start_pose ? *start_pose : XFORM(),
			local_animation
		);
	}
	else
	{
//		start_pose		= &renderable.xform;
		if( m_anim_mov_ctrl )
			destroy_anim_mov_ctrl();

		VERIFY2			(
			start_pose,
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);

		VERIFY2			(
			!animation_movement(),
			make_string(
				"start pose hasn't been specified for animation [%s][%s]",
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).first,
				smart_cast<IKinematicsAnimated&>(*Visual()).LL_MotionDefName_dbg(b->motionID).second
			)
		);
		
		VERIFY			(Visual());
		IKinematics		*K = Visual( )->dcast_PKinematics( );
		VERIFY			( K );

		m_anim_mov_ctrl	= xr_new<animation_movement_controller>( &XFORM(), *start_pose, K, b ); 
	}
}

void CGameObject::destroy_anim_mov_ctrl	()
{
	xr_delete			( m_anim_mov_ctrl );
}

IC	bool similar						(const Fmatrix &_0, const Fmatrix &_1, const float &epsilon = EPS)
{
	if (!_0.i.similar(_1.i,epsilon))
		return						(false);

	if (!_0.j.similar(_1.j,epsilon))
		return						(false);

	if (!_0.k.similar(_1.k,epsilon))
		return						(false);

	if (!_0.c.similar(_1.c,epsilon))
		return						(false);

	// note: we do not compare projection here
	return							(true);
}

void CGameObject::UpdateCL			()
{
	inherited::UpdateCL				();
	
//	if (!is_ai_obstacle())
//		return;
	
	if (H_Parent())
		return;

	if (similar(XFORM(),m_previous_matrix,EPS))
		return;

	on_matrix_change				(m_previous_matrix);
	m_previous_matrix				= XFORM();
}

void CGameObject::on_matrix_change	(const Fmatrix &previous)
{
	obstacle().on_move				();
}

#ifdef DEBUG

void render_box						(IRenderVisual *visual, const Fmatrix &xform, const Fvector &additional, bool draw_child_boxes, const u32 &color)
{
	CDebugRenderer			&renderer = Level().debug_renderer();
	IKinematics				*kinematics = smart_cast<IKinematics*>(visual);
	VERIFY					(kinematics);
	u16						bone_count = kinematics->LL_BoneCount();
	VERIFY					(bone_count);
	u16						visible_bone_count = kinematics->LL_VisibleBoneCount();
	if (!visible_bone_count)
		return;

	Fmatrix					matrix;
	Fvector					*points = (Fvector*)_alloca(visible_bone_count*8*sizeof(Fvector));
	Fvector					*I = points;
	for (u16 i=0; i<bone_count; ++i) {
		if (!kinematics->LL_GetBoneVisible(i))
			continue;
		
		const Fobb			&obb = kinematics->LL_GetData(i).obb;
		if (fis_zero(obb.m_halfsize.square_magnitude())) {
			VERIFY			(visible_bone_count > 1);
			--visible_bone_count;
			continue;
		}

		Fmatrix				Mbox;
		obb.xform_get		(Mbox);

		const Fmatrix		&Mbone = kinematics->LL_GetBoneInstance(i).mTransform;
		Fmatrix				X;
		matrix.mul_43		(xform,X.mul_43(Mbone,Mbox));

		Fvector				half_size = Fvector().add(obb.m_halfsize,additional);
		matrix.mulB_43		(Fmatrix().scale(half_size));

		if (draw_child_boxes)
			renderer.draw_obb	(matrix,color);

		static const Fvector	local_points[8] = {
			Fvector().set(-1.f,-1.f,-1.f),
			Fvector().set(-1.f,-1.f,+1.f),
			Fvector().set(-1.f,+1.f,+1.f),
			Fvector().set(-1.f,+1.f,-1.f),
			Fvector().set(+1.f,+1.f,+1.f),
			Fvector().set(+1.f,+1.f,-1.f),
			Fvector().set(+1.f,-1.f,+1.f),
			Fvector().set(+1.f,-1.f,-1.f)
		};
		
		for (u32 i=0; i<8; ++i, ++I)
			matrix.transform_tiny	(*I,local_points[i]);
	}

	VERIFY						(visible_bone_count);
	if (visible_bone_count == 1) {
		renderer.draw_obb		(matrix,color);
		return;
	}

	VERIFY						((I - points) == (visible_bone_count*8));
	MagicBox3					box = MagicMinBox(visible_bone_count*8,points);
	box.ComputeVertices			(points);
	
	Fmatrix						result;
	result.identity				();

	result.c					= box.Center();

	result.i.sub(points[3],points[2]).normalize();
	result.j.sub(points[2],points[1]).normalize();
	result.k.sub(points[2],points[6]).normalize();

	Fvector						scale;
	scale.x						= points[3].distance_to(points[2])*.5f;
	scale.y						= points[2].distance_to(points[1])*.5f;
	scale.z						= points[2].distance_to(points[6])*.5f;
	result.mulB_43				(Fmatrix().scale(scale));

	renderer.draw_obb			(result,color);
}

void CGameObject::OnRender			()
{
	if (!ai().get_level_graph())
		return;

	CDebugRenderer					&renderer = Level().debug_renderer();
	if (/**bDebug && /**/Visual()) {
		float						half_cell_size = 1.f*ai().level_graph().header().cell_size()*.5f;
		Fvector						additional = Fvector().set(half_cell_size,half_cell_size,half_cell_size);

		render_box					(Visual(),XFORM(),Fvector().set(0.f,0.f,0.f),true,color_rgba(0,0,255,255));
		render_box					(Visual(),XFORM(),additional,false,color_rgba(0,255,0,255));
	}

	if (0) {
		Fvector						bc,bd; 
		Visual()->getVisData().box.get_CD	(bc,bd);
		Fmatrix						M = Fidentity;
		float						half_cell_size = ai().level_graph().header().cell_size()*.5f;
		bd.add						(Fvector().set(half_cell_size,half_cell_size,half_cell_size));
		M.scale						(bd);
		Fmatrix						T = XFORM();
		T.c.add						(bc);
		renderer.draw_obb			(T,bd,color_rgba(255,255,255,255));
	}
}
#endif // DEBUG
