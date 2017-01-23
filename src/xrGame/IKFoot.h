#pragma once

#include "ik_calculate_data.h"
#include "ik_foot_collider.h"

struct local_vector
{
	Fvector v;
	u16		bone;
	local_vector(): bone( u16(-1) ), v( Fvector().set( 0, 0, 0 ) ){ }
};

class	IKinematics;
struct  SCalculateData;
struct  SIKCollideData;
class	CGameObject;


class	CIKFoot
{
public:
						CIKFoot						( );
	void				Create						( IKinematics	*K, LPCSTR section, u16 bones[4] );
IC	void				set_ref_bone				( u16 ref_bone ) { m_ref_bone =  ref_bone; }
	void				set_ref_bone				( );
	u16					get_ref_bone				( const Fmatrix & foot_transform, const Fmatrix &toe_transform ) const;
public:
IC	Fvector				&ToePosition				( Fvector &toe_position ) const;
IC	Fvector				&HeelPosition				( Fvector &heel_position ) const;
IC	u16					ref_bone					( ) const { return m_ref_bone; }
	Fmatrix				&ref_bone_to_foot			( Fmatrix &foot, const Fmatrix &ref_bone ) const;
	Fmatrix				&ref_bone_to_foot			( Fmatrix &ref_bone ) const;
private:
public:
IC	Fvector				&FootNormal					( Fvector &foot_normal ) const;
private:
IC	Fvector				&get_local_vector			( Fvector &v, const local_vector &lv )const;
IC	Fvector&			get_local_vector			( u16 bone, Fvector &v, const local_vector &lv )const;
	Fmatrix				&ref_bone_to_foot_transform	( Fmatrix& m ) const;
	Fmatrix				&foot_to_ref_bone_transform	( Fmatrix& m ) const;
	Fmatrix				&foot_to_ref_bone			( Fmatrix &ref_bone, const Fmatrix &foot ) const;
	Fmatrix				&foot_to_ref_bone			( Fmatrix &foot ) const;
public:
	bool				GetFootStepMatrix			( ik_goal_matrix	&m, const SCalculateData& cd, const  SIKCollideData &cld, bool collide, bool rotate )const;
	bool				GetFootStepMatrix			( ik_goal_matrix &m, const Fmatrix &gl_nim, const  SIKCollideData &cld, bool collide, bool rotate, bool make_shift = true )const;
	void				SetFootGeom					( ik_foot_geom &fg, const Fmatrix &ref_bone, const Fmatrix& object_matrix ) const;
	void				Collide						( SIKCollideData &cld,  ik_foot_collider	&collider, const Fmatrix &ref_bone, const Fmatrix& object_matrix, CGameObject *O, bool foot_step ) const;
private:

ik_goal_matrix::e_collide_state		CollideFoot		( float angle, float &out_angle, const Fvector &global_toe, const Fvector	&foot_normal,  const Fvector	&global_bone_pos, const Fplane &p, const Fvector &ax )const;
ik_goal_matrix::e_collide_state		rotate			( Fmatrix &xm, const Fplane& p, const Fvector &normal, const Fvector &global_point, bool collide )const;

IC	bool				make_shift					( Fmatrix &xm, const Fvector &cl_point, bool collide, const Fplane &p, const Fvector &pick_dir  )const;

private:
	void				set_toe						( u16 bones[4] );
public:
IC  IKinematics	*		Kinematics					( )const			{ return m_K; }

private:
	IKinematics			*m_K;

	local_vector		m_toe_position;
	local_vector		m_heel_position;

	local_vector		m_foot_normal;
	local_vector		m_foot_direction;

	Fmatrix				m_bind_b2_to_b3;
	float				m_foot_width;
	u16					m_ref_bone;
	u16					m_foot_bone_id;
	u16					m_toe_bone_id;
};

#include	"IKFoot_inl.h"