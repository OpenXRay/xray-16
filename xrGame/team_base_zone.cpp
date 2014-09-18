////////////////////////////////////////////////////////////////////////////
//	Module 		: team_base_zone.h
//	Created 	: 27.04.2004
//  Modified 	: 27.04.2004
//	Author		: Dmitriy Iassenev
//	Description : Team base zone object
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "team_base_zone.h"
#include "xrserver_objects_alife_monsters.h"
#include "hit.h"
#include "Actor.h"
#include "level.h"
#include "xrserver.h"
#include "game_cl_base.h"
#include "map_manager.h"
#include "map_location.h"
#include "../xrEngine/IGame_Persistent.h"
#include "../xrengine/xr_collide_form.h"
#ifdef DEBUG
#	include "debug_renderer.h"
#endif

CTeamBaseZone::CTeamBaseZone		()
{
}

CTeamBaseZone::~CTeamBaseZone		()
{
}

void CTeamBaseZone::reinit			()
{
	inherited::reinit		();
}

void CTeamBaseZone::Center			(Fvector &C) const
{
	XFORM().transform_tiny	(C,CFORM()->getSphere().P);
}

float CTeamBaseZone::Radius			() const
{
	return						(CFORM()->getRadius());
}

BOOL CTeamBaseZone::net_Spawn	(CSE_Abstract* DC) 
{
	CCF_Shape					*l_pShape = xr_new<CCF_Shape>(this);
	collidable.model			= l_pShape;

	CSE_Abstract				*l_tpAbstract = (CSE_Abstract*)(DC);
	CSE_ALifeTeamBaseZone		*l_tpALifeScriptZone = smart_cast<CSE_ALifeTeamBaseZone*>(l_tpAbstract);
	R_ASSERT					(l_tpALifeScriptZone);

	feel_touch.clear			();

	for (u32 i=0; i < l_tpALifeScriptZone->shapes.size(); ++i) {
		CSE_Shape::shape_def	&S = l_tpALifeScriptZone->shapes[i];
		switch (S.type) {
			case 0 : {
				l_pShape->add_sphere(S.data.sphere);
				break;
			}
			case 1 : {
				l_pShape->add_box(S.data.box);
				break;
			}
		}
	}

	m_Team = l_tpALifeScriptZone->m_team;

	BOOL						bOk = inherited::net_Spawn(DC);
	if (bOk) {
		l_pShape->ComputeBounds	();
		Fvector					P;
		XFORM().transform_tiny	(P,CFORM()->getSphere().P);
		setEnabled				(TRUE);
	}

	if (GameID() != eGameIDSingle && !g_dedicated_server)
	{
		char BaseMapLocation[1024];
		xr_sprintf (BaseMapLocation, "mp_team_base_%d_location", m_Team);
		(Level().MapManager().AddMapLocation(BaseMapLocation,ID()))->EnablePointer();
		
	};

	return						(bOk);
}

void CTeamBaseZone::net_Destroy			()
{
	if(!g_dedicated_server)
		Level().MapManager().OnObjectDestroyNotify(ID());

	inherited::net_Destroy();
};

void CTeamBaseZone::shedule_Update(u32 dt)
{
	inherited::shedule_Update	(dt);
	
	const Fsphere				&s = CFORM()->getSphere();
	Fvector						P;
	XFORM().transform_tiny		(P,s.P);
	feel_touch_update			(P,s.R);
}

void CTeamBaseZone::feel_touch_new	(CObject *tpObject)
{
	if(OnServer() && smart_cast<CActor*>(tpObject))
	{
		NET_Packet			P_;

		u_EventGen			(P_,GE_GAME_EVENT,ID()	);
		P_.w_u16			(GAME_EVENT_PLAYER_ENTER_TEAM_BASE);
		P_.w_u16			( tpObject->ID() );
		P_.w_u8				( GetZoneTeam() );
		u_EventSend			(P_,net_flags(TRUE,TRUE));
	};
}

void CTeamBaseZone::feel_touch_delete	(CObject *tpObject)
{
	if(OnServer() && smart_cast<CActor*>(tpObject))
	{
		NET_Packet			P_;
		u_EventGen			(P_,GE_GAME_EVENT,ID()	);
		P_.w_u16			(GAME_EVENT_PLAYER_LEAVE_TEAM_BASE );
		P_.w_u16			( tpObject->ID() );
		P_.w_u8				( GetZoneTeam() );
		u_EventSend			(P_,net_flags(TRUE,TRUE));
	};
}

BOOL CTeamBaseZone::feel_touch_contact	(CObject* O)
{
	CActor*	pActor = smart_cast<CActor*>(O);
	if (!pActor) return (FALSE);
	return ((CCF_Shape*)CFORM())->Contact(O);
}

#ifdef DEBUG
extern	Flags32	dbg_net_Draw_Flags;
void CTeamBaseZone::OnRender() 
{
	if(!bDebug) return;
	if (!(dbg_net_Draw_Flags.is_any(dbg_draw_teamzone))) return;
//	RCache.OnFrameEnd();
	Fvector l_half; l_half.set(.5f, .5f, .5f);
	Fmatrix l_ball, l_box;
	xr_vector<CCF_Shape::shape_def> &l_shapes = ((CCF_Shape*)CFORM())->Shapes();
	xr_vector<CCF_Shape::shape_def>::iterator l_pShape;
	
	for(l_pShape = l_shapes.begin(); l_shapes.end() != l_pShape; ++l_pShape) 
	{
		switch(l_pShape->type)
		{
		case 0:
			{
                Fsphere &l_sphere = l_pShape->data.sphere;
				l_ball.scale(l_sphere.R, l_sphere.R, l_sphere.R);
				Fvector l_p; XFORM().transform(l_p, l_sphere.P);
				l_ball.translate_add(l_p);
				Level().debug_renderer().draw_ellipse(l_ball, D3DCOLOR_XRGB(0,255,255));
			}
			break;
		case 1:
			{
				l_box.mul(XFORM(), l_pShape->data.box);
				Level().debug_renderer().draw_obb(l_box, l_half, D3DCOLOR_XRGB(0,255,255));
			}
			break;
		}
	}
}
#endif
