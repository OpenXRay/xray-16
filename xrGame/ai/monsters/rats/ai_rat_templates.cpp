////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_rat_templates.cpp
//	Created 	: 23.07.2002
//  Modified 	: 07.11.2002
//	Author		: Dmitriy Iassenev
//	Description : Templates for monster "Rat"
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_rat.h"
#include "../../ai_monsters_misc.h"
#include "../../../game_graph.h"
#include "../../../magic_box3.h"
#include "../../../../Include/xrRender/RenderVisual.h"
#include "../../../ai_object_location.h"
#include "../../../level_graph.h"
#include "../../../movement_manager.h"
#include "../../../location_manager.h"
#include "../../../level.h"
#include "../../../random32.h"
#include "../../../ai_space.h"
#include "../../../restricted_object.h"
#include "../../../patrol_path.h"
#include "../../../debug_renderer.h"
#include "../ai_monster_squad_manager.h"
#include "../ai_monster_squad.h"

IC bool CAI_Rat::bfCheckIfOutsideAIMap(Fvector &tTemp1)
{
	u32 dwNewNode = ai_location().level_vertex_id();
	const CLevelGraph::CVertex *tpNewNode = ai_location().level_vertex();
	CLevelGraph::CPosition	QueryPos;
	if (!ai().level_graph().valid_vertex_position(tTemp1))
		return	(false);
	ai().level_graph().vertex_position(QueryPos,tTemp1);
	if (!ai().level_graph().valid_vertex_id(dwNewNode) || !ai().level_graph().inside(*ai_location().level_vertex(),QueryPos)) {
		dwNewNode = ai().level_graph().vertex(ai_location().level_vertex_id(),tTemp1);
		tpNewNode = ai().level_graph().vertex(dwNewNode);
	}
	return(!ai().level_graph().valid_vertex_id(dwNewNode) || !ai().level_graph().inside(*tpNewNode,QueryPos));
};

void CAI_Rat::fire	(bool const &bFire)
{
	if (bFire) {
		m_bFiring = true;
		m_tAction = eRatActionAttackBegin;
	}
	else {
		m_bFiring = false;
		m_tAction = eRatActionAttackEnd;
	}
}

void CAI_Rat::movement_type	(float const &fSpeed)
{
//	StandUp();
	m_bMoving = _abs(fSpeed) > EPS_L;
	m_fSpeed = m_fCurSpeed = fSpeed;
}

void CAI_Rat::select_speed	()
{
	Fvector					tTemp1, tTemp2;
	tTemp1.sub				(m_tGoalDir,Position());
	tTemp1.normalize_safe	();
	float					y,p;
	tTemp1.getHP			(y,p);
	tTemp2					= XFORM().k;
	tTemp2.normalize_safe	();
	float					fAngle = tTemp1.dotproduct(tTemp2);
	clamp					(fAngle,-.99999f,.99999f);
	fAngle					= acosf(fAngle);
	
	if (_abs(m_fSpeed - m_fMinSpeed) <= EPS_L)	{
		if (fAngle >= 2*PI_DIV_3) {
			m_fSpeed = 0;
			m_fASpeed = m_fNullASpeed;
			movement().m_body.target.yaw = -y;
		}
		else 
		{
			m_fSpeed = m_fMinSpeed;
			m_fASpeed = m_fMinASpeed;
		}
	}
	else
		if (_abs(m_fSpeed - m_fMaxSpeed) <= EPS_L)	{
			if (fAngle >= 2*PI_DIV_3) {
				m_fSpeed = 0;
				m_fASpeed = m_fNullASpeed;
				movement().m_body.target.yaw = -y;
			}
			else
				if (fAngle >= PI_DIV_2) {
					m_fSpeed = m_fMinSpeed;
					m_fASpeed = m_fMinASpeed;
				}
				else {
					m_fSpeed = m_fMaxSpeed;
					m_fASpeed = m_fMaxASpeed;
				}
		}
		else
			if (_abs(m_fSpeed - m_fAttackSpeed) <= EPS_L)	{
//				if (fAngle >= 2*PI_DIV_3) {
//					m_fSpeed = 0;
//					m_fASpeed = m_fNullASpeed;
//					movement().m_body.target.yaw = -y;
//				}
//				else
					if (fAngle >= PI_DIV_2) {
						m_fSpeed = m_fMinSpeed;
						m_fASpeed = m_fMinASpeed;
					}
					else
						if (fAngle >= PI_DIV_4) {
							m_fSpeed = m_fMaxSpeed;
							m_fASpeed = m_fMaxASpeed;
						}
						else {
							m_fSpeed = m_fAttackSpeed;
							m_fASpeed = m_fAttackASpeed;
						}
			}
			else {
				movement().m_body.target.yaw = -y;
				m_fSpeed = 0;
				m_fASpeed = m_fNullASpeed;
			}
	
	tTemp2 = XFORM().k;
	tTemp2.normalize_safe();
	
	tTemp1 = Position();
	tTemp1.mad(tTemp2,1*m_fSpeed*m_fTimeUpdateDelta);
	if (bfCheckIfOutsideAIMap(tTemp1)) {
		tTemp1 = Position();
		if (_abs(m_fSpeed - m_fAttackSpeed) < EPS_L) {
			tTemp1.mad(tTemp2,1*m_fMaxSpeed*m_fTimeUpdateDelta);
			if (bfCheckIfOutsideAIMap(tTemp1)) {
				m_fSpeed = m_fMinSpeed;
				m_fASpeed = m_fMinASpeed;
			}
			else {
				m_fSpeed = m_fMaxSpeed;
				m_fASpeed = m_fMaxASpeed;
			}
		}
		else 
		{
			m_fSpeed = m_fMinSpeed;
			m_fASpeed = m_fMinASpeed;
		}
	}
}

void CAI_Rat::make_turn()
{
	m_fSpeed			= m_fCurSpeed = 0.f;
	if (m_bFiring && (angle_difference(movement().m_body.target.yaw,movement().m_body.current.yaw) < PI_DIV_6)) {
//		movement().m_body.speed	= 0.f;
		return;
	}

//	Msg					("%6d : Rat %s, %f -> %f [%f]",Device.dwTimeGlobal,*cName(),movement().m_body.current.pitch,movement().m_body.target.pitch,get_custom_pitch_speed(0.f));

	m_turning			= true;
	movement().m_body.speed		= PI_MUL_2;

	Fvector				tSavedPosition = Position();
	m_tHPB.x			= -movement().m_body.current.yaw;
	m_tHPB.y			= -movement().m_body.current.pitch;

	XFORM().setHPB		(m_tHPB.x,m_tHPB.y,0.f);//m_tHPB.z);
	Position()			= tSavedPosition;
}

void CAI_Rat::set_firing(bool b_val)
{
	m_bFiring = b_val;
}

bool CAI_Rat::calc_node(Fvector const &next_position)
{
	u32 dwNewNode = ai_location().level_vertex_id();
	const CLevelGraph::CVertex *tpNewNode = ai_location().level_vertex();
	CLevelGraph::CPosition	QueryPos;
	bool					a = !ai().level_graph().valid_vertex_id(dwNewNode) || !ai().level_graph().valid_vertex_position(next_position);
	if (!a) {
		ai().level_graph().vertex_position	(QueryPos,next_position);
		a					= !ai().level_graph().inside(*ai_location().level_vertex(),QueryPos);
	}
	if (a) {
		dwNewNode = ai().level_graph().vertex(ai_location().level_vertex_id(),next_position);
		tpNewNode = ai().level_graph().vertex(dwNewNode);
	}
	if (ai().level_graph().valid_vertex_id(dwNewNode) && ai().level_graph().inside(*tpNewNode,QueryPos)) {
		m_tNewPosition.y =  ai().level_graph().vertex_plane_y(*tpNewNode,next_position.x,next_position.z);
		if (movement().restrictions().accessible(m_tNewPosition)||!movement().restrictions().accessible(Position()))
			return true;
	}
	return false;
}

Fvector CAI_Rat::calc_position()
{
	Fvector				tSavedPosition = Position();
	SRotation			tSavedTorsoTarget = movement().m_body.target;

	// Update position and orientation of the planes
	float fAT = m_fASpeed * m_fTimeUpdateDelta;

	Fvector& tDirection = XFORM().k;

	// Tweak orientation based on last position and goal
	Fvector tOffset;
	tOffset.sub(m_tGoalDir,Position());

	if (!m_bStraightForward) {
		if (tOffset.y > 1.0) {			// We're too low
			m_tHPB.y += fAT;
			if (m_tHPB.y > 0.8f)	
				m_tHPB.y = 0.8f;
		}
		else 
			if (tOffset.y < -1.0) {	// We're too high
				m_tHPB.y -= fAT;
				if (m_tHPB.y < -0.8f)
					m_tHPB.y = -0.8f;
			}
			else							// Add damping
				m_tHPB.y *= 0.95f;
	}

	tDirection.normalize_safe	();
	tOffset.normalize_safe		();

	float fDot = tDirection.dotproduct(tOffset);
	float fSafeDot = fDot;

	fDot = (1.0f-fDot)/2.0f * fAT * 10.0f;

	tOffset.crossproduct(tOffset,tDirection);

	if (m_bStraightForward) {
		if (tOffset.y > 0.01f) {
			if (fSafeDot > .95f)
				m_fDHeading = 0.f;
			else
				if (fSafeDot > 0.75f)
					m_fDHeading = .10f;
			m_fDHeading = ( m_fDHeading * 9.0f + fDot )*0.1f;
		}
		else 
			if (tOffset.y < 0.01f) {
				if (fSafeDot > .95f)
					m_fDHeading = 0.f;
				else
					if (fSafeDot > 0.75f)
						m_fDHeading = -.10f;
				m_fDHeading = (m_fDHeading*9.0f - fDot)*0.1f;
			}
	}
	else {
		if (tOffset.y > 0.01f)		
			m_fDHeading = ( m_fDHeading * 9.0f + fDot )*0.1f;
		else 
			if (tOffset.y < 0.01f)
				m_fDHeading = (m_fDHeading*9.0f - fDot)*0.1f;
	}


	m_tHPB.x  +=  m_fDHeading;
		CLevelGraph::SContour	contour;
	ai().level_graph().contour(contour, ai_location().level_vertex_id());
	
	Fplane  P;
	P.build(contour.v1,contour.v2,contour.v3);

	Fvector position_on_plane;
	P.project(position_on_plane,Position());

	// находим проекцию точки, лежащей на векторе текущего направления
	Fvector dir_point, proj_point;
	dir_point.mad(position_on_plane, Direction(), 1.f);
	P.project(proj_point,dir_point);
	
	// получаем искомый вектор направления
	Fvector target_dir;
	target_dir.sub(proj_point,position_on_plane);

	float yaw,pitch;
	target_dir.getHP(yaw,pitch);

	//movement().m_body.target.pitch = -pitch;
	m_newPitch = -pitch;

	m_tHPB.x			= angle_normalize_signed(m_tHPB.x);
	m_tHPB.y			= -movement().m_body.current.pitch;
	return tSavedPosition.mad	(tDirection,m_fSpeed*m_fTimeUpdateDelta);
}

void CAI_Rat::set_position(Fvector m_position)
{
	XFORM().setHPB					(m_tHPB.x,m_tHPB.y,0.f);
	Position()						= m_position;
}

void CAI_Rat::set_pitch(float pitch, float yaw)
{
	movement().m_body.target.pitch	= pitch;
	movement().m_body.target.yaw	= yaw;
}


void CAI_Rat::move	(bool bCanAdjustSpeed, bool bStraightForward)
{
	m_bCanAdjustSpeed	= bCanAdjustSpeed;
	m_bStraightForward	= bStraightForward;


	Fvector				tSafeHPB = m_tHPB;
	Fvector				tSavedPosition = Position();
	SRotation			tSavedTorsoTarget = movement().m_body.target;
	float fSavedDHeading = m_fDHeading;

	if (bCanAdjustSpeed)
		select_speed	();

	if ((angle_difference(movement().m_body.target.yaw, movement().m_body.current.yaw) > PI_DIV_6)){
		make_turn	();
		return;
	}

	if (fis_zero(m_fSpeed))
		return;

	m_fCurSpeed		= m_fSpeed;

	if (m_bNoWay)
	{
		m_tNewPosition	= m_tOldPosition;
	} else {
		m_tNewPosition = calc_position();
	}
	if (calc_node(m_tNewPosition))
	{
		set_position(m_tNewPosition);
		set_pitch(m_newPitch, -m_tHPB.x);
		m_tOldPosition	= tSavedPosition;
		m_bNoWay		= false;
	}
	else {
		m_fSafeSpeed	= m_fSpeed = EPS_S;
		m_bNoWay		= true;
		m_tHPB			= tSafeHPB;
		set_position(tSavedPosition);
		movement().m_body.target	= tSavedTorsoTarget;
		m_fDHeading		= fSavedDHeading;
	}
	if (m_bNoWay && (!m_turning || (angle_difference(movement().m_body.target.yaw, movement().m_body.current.yaw) < EPS_L))) {
		if ((Device.dwTimeGlobal - m_previous_query_time > TIME_TO_RETURN) || (!m_previous_query_time)) {
			movement().m_body.target.yaw = movement().m_body.current.yaw + PI;
			movement().m_body.target.yaw = angle_normalize(movement().m_body.target.yaw);
			Fvector tTemp;
			tTemp.setHP(-movement().m_body.target.yaw ,-movement().m_body.target.pitch);
			if (m_bStraightForward)
			{
				tTemp.mul(100.f);
			}
			
			if (!m_walk_on_way) m_tGoalDir.add(Position(),tTemp);

			m_previous_query_time = Device.dwTimeGlobal;
		}
		if (!m_walk_on_way) make_turn		();
	}
	m_turning			= false;
}

void CAI_Rat::select_next_home_position	()
{
	GameGraph::_GRAPH_ID	tGraphID		= m_next_graph_point;
	CGameGraph::const_iterator	i,e;
	ai().game_graph().begin		(tGraphID,i,e);
	int					iPointCount		= (int)movement().locations().vertex_types().size();
	int					iBranches		= 0;
	for ( ; i != e; ++i)
		for (int j=0; j<iPointCount; ++j)
			if (ai().game_graph().mask(movement().locations().vertex_types()[j].tMask,ai().game_graph().vertex((*i).vertex_id())->vertex_type()) && ((*i).vertex_id() != m_current_graph_point))
				++iBranches;
	ai().game_graph().begin		(tGraphID,i,e);
	if (!iBranches) {
		for ( ; i != e; ++i) {
			for (int j=0; j<iPointCount; ++j)
				if (ai().game_graph().mask(movement().locations().vertex_types()[j].tMask,ai().game_graph().vertex((*i).vertex_id())->vertex_type())) {
					m_current_graph_point	= m_next_graph_point;
					m_next_graph_point	= (*i).vertex_id();
					m_time_to_change_graph_point	= Device.dwTimeGlobal + ::Random32.random(60000) + 60000;
					return;
				}
		}
	}
	else {
		int iChosenBranch = ::Random.randI(0,iBranches);
		iBranches = 0;
		for ( ; i != e; ++i) {
			for (int j=0; j<iPointCount; ++j)
				if (ai().game_graph().mask(movement().locations().vertex_types()[j].tMask,ai().game_graph().vertex((*i).vertex_id())->vertex_type()) && ((*i).vertex_id() != m_current_graph_point)) {
					if (iBranches == iChosenBranch) {
						m_current_graph_point	= m_next_graph_point;
						m_next_graph_point	= (*i).vertex_id();
						m_time_to_change_graph_point	= Device.dwTimeGlobal + ::Random32.random(60000) + 60000;
						return;
					}
					++iBranches;
				}
		}
	}
}

bool CAI_Rat::can_stand_in_position()
{
	xr_vector<CObject*>					tpNearestList								; 
	//float m_radius = Radius();
	Level().ObjectSpace.GetNearest		(tpNearestList, Position(), 0.2f, this)	; 
	if (tpNearestList.empty())
		return							(true);

	Fvector								c, d, C2;
	Visual()->getVisData().box.get_CD			(c,d);
	Fmatrix								M = XFORM();
	M.transform_tiny					(C2,c);
	M.c									= C2;
	MagicBox3							box(M,d);

	xr_vector<CObject*>::iterator		I = tpNearestList.begin();
	xr_vector<CObject*>::iterator		E = tpNearestList.end();
	for ( ; I != E; ++I) {
		if (!smart_cast<CAI_Rat*>(*I))
			continue;

		(*I)->Visual()->getVisData().box.get_CD	(c,d);
		M								= (*I)->XFORM();
		M.transform_tiny				(C2,c);
		M.c								= C2;

		if (box.intersects(MagicBox3(M,d)))
			return						(false);
	}
	return								(true);
}

bool CAI_Rat::can_stand_here	()
{
	xr_vector<CObject*>					tpNearestList								; 
	Level().ObjectSpace.GetNearest		(tpNearestList, Position(),Radius(),this)	; 
	//xr_vector<CObject*>				&tpNearestList = Level().ObjectSpace.q_nearest; 
	if (tpNearestList.empty())
		return							(true);

	Fvector								c, d, C2;
	Visual()->getVisData().box.get_CD			(c,d);
	Fmatrix								M = XFORM();
	M.transform_tiny					(C2,c);
	M.c									= C2;
	MagicBox3							box(M,d);

	xr_vector<CObject*>::iterator		I = tpNearestList.begin();
	xr_vector<CObject*>::iterator		E = tpNearestList.end();
	for ( ; I != E; ++I) {
		if (!smart_cast<CAI_Rat*>(*I))
			continue;
		
		(*I)->Visual()->getVisData().box.get_CD	(c,d);
		M								= (*I)->XFORM();
		M.transform_tiny				(C2,c);
		M.c								= C2;
		
		if (box.intersects(MagicBox3(M,d)))
			return						(false);
	}
	return								(true);
}

Fvector CAI_Rat::get_next_target_point()
{
	if (!m_path)
	{
		m_walk_on_way = false;
		m_current_way_point = u32(-1);
		return Position();
	}

	if (m_current_way_point == u32(-1))
	{
		m_walk_on_way = false;
		return Position();
	}

	const CPatrolPath::CVertex *vertex = m_path->vertex(m_current_way_point);

	if (Position().distance_to(ai().level_graph().vertex_position(vertex->data().level_vertex_id())) < 1.5f){

		monster_squad().get_squad(this)->SetLeader(this);

		m_current_way_point++;

		if (m_current_way_point == m_path->vertex_count()) m_current_way_point = 0;
		
		vertex = m_path->vertex(m_current_way_point);
	}

	return (ai().level_graph().vertex_position(vertex->data().level_vertex_id()));
}

#ifdef _DEBUG
void CAI_Rat::draw_way()
{
	if (!m_path) return;
	const CPatrolPath::CVertex *vertex;
	Fvector P1, P2;
	Fmatrix m_sphere;

	for (u32 i=1; i<m_path->vertex_count(); i++) {
		vertex = m_path->vertex(i-1);
		P1 = ai().level_graph().vertex_position(vertex->data().level_vertex_id());
		vertex = m_path->vertex(i);
		P2 = ai().level_graph().vertex_position(vertex->data().level_vertex_id());
		if (!fis_zero(P1.distance_to_sqr(P2),EPS_L))
			Level().debug_renderer().draw_line			(Fidentity,P1,P2,D3DCOLOR_XRGB(0,0,255));
		m_sphere.identity();
		m_sphere.scale(0.25, 0.25, 0.25);
		m_sphere.translate_add(P2);
		Level().debug_renderer().draw_ellipse(m_sphere, D3DCOLOR_XRGB(0,255,255));
		//Level().debug_renderer().draw_aabb			(P1,0.5f,0.5f,0.5f,D3DCOLOR_XRGB(0,255,255));
	}

	vertex = m_path->vertex(0);
	P1 = ai().level_graph().vertex_position(vertex->data().level_vertex_id());
	vertex = m_path->vertex(m_path->vertex_count() - 1);
	P2 = ai().level_graph().vertex_position(vertex->data().level_vertex_id());
	if (!fis_zero(P1.distance_to_sqr(P2),EPS_L))
		Level().debug_renderer().draw_line			(Fidentity,P1,P2,D3DCOLOR_XRGB(0,0,255));
	m_sphere.identity();
	m_sphere.scale(0.25, 0.25, 0.25);
	m_sphere.translate_add(P2);
	Level().debug_renderer().draw_ellipse(m_sphere, D3DCOLOR_XRGB(0,255,255));
	//Level().debug_renderer().draw_aabb			(P1,0.5f,0.5f,0.5f,D3DCOLOR_XRGB(0,255,255));
}
#endif
