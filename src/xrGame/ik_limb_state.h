#pragma	once

#include	"ik_calculate_data.h"

#include	"../include/xrrender/Kinematics.h"

class CIKLimb;
IC bool state_valide( const calculate_state &prev_state );

class ik_limb_state
{

		calculate_state				state;
const	CIKLimb					   *limb;
#ifdef IK_DBG_STATE_SEQUENCE
		xr_vector<calculate_state>	sv_state;
#endif

public:
						ik_limb_state		( ): state( ), limb( 0 )	{ }
						ik_limb_state		( const CIKLimb *l, const ik_limb_state& s ): state( s.state ), limb( l )	{ }
				void	set_limb			( CIKLimb *l )				;

IC				void	save_new_state		( const calculate_state	&s )	
{

#ifdef IK_DBG_STATE_SEQUENCE
	if( sv_state.size()>130 )
		sv_state.erase( sv_state.begin( ) );
	sv_state.push_back( state );
#endif
	state = s; 

}
private:
IC		const	Fmatrix			&anim_pos			() const {	 return state.anim_pos; }
IC		const	ik_goal_matrix	&goal				() const {	 return state.goal; }
IC		const	ik_goal_matrix	&blend_to			() const {	 return state.blend_to; }
IC		const	Fvector			&pick				() const {	 return state.pick; }
public:
				Fmatrix			&anim_pos			( Fmatrix			&m )	const ; 
				ik_goal_matrix	&goal				( ik_goal_matrix	&m )	const ;
				ik_goal_matrix	&blend_to			( ik_goal_matrix	&m )	const ;
				Fmatrix 		&to_ref_bone		( Fmatrix			&m )	const ;
				ik_goal_matrix 	&to_ref_bone		( ik_goal_matrix	&m )	const ;
				Fvector			&pick				( Fvector			&v )	const ;
			
IC				u16				ref_bone			() const {	 return state.ref_bone; }
IC				bool			foot_step			() const {	 return state.foot_step; }
IC				bool			blending			() const {	 return state.blending; }
IC				bool			valide				() const {	 return state_valide( state ); }

IC				void	get_calculate_state ( calculate_state	&s ) const	
{
		s.calc_time			= Device.dwTimeGlobal;
		s.blending			= valide() && ( state.blending || state.foot_step != s.foot_step ); //prev_state.state !=calculate_state::not_definite &&

		s.collide_pos		= state.collide_pos;
		Fmatrix		cl_pos  = state.collide_pos.get();
		to_ref_bone			( cl_pos );
		s.collide_pos.set	( cl_pos, state.collide_pos.collide_state() );

		s.speed_blend_l 	= state.speed_blend_l;
		s.speed_blend_a 	= state.speed_blend_a;
		s.unstuck_time		= state.unstuck_time;
#ifdef DEBUG
		s.count				= state.count;
#endif
}

};

#ifdef	DEBUG
extern	bool dbg_always_valide	;
#endif

IC bool state_valide( const calculate_state &prev_state )
{
#ifdef	DEBUG
	if( dbg_always_valide )
		return	true;
#endif
	return (Device.dwTimeGlobal <= (prev_state.calc_time + UCalc_Interval + Device.dwTimeDelta));
}

IC bool state_valide( const ik_limb_state& s )
{
	return	s.valide();
}

